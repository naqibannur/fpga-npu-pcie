#!/usr/bin/env python3

"""
FPGA NPU Project - Automated Testing Framework
Comprehensive test orchestration and reporting system
"""

import argparse
import json
import logging
import os
import subprocess
import sys
import time
import threading
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import concurrent.futures
import shutil
import tempfile

# Test result data structures
@dataclass
class TestResult:
    name: str
    status: str  # PASS, FAIL, SKIP, ERROR
    duration: float
    message: str = ""
    output: str = ""
    error_output: str = ""

@dataclass
class TestSuite:
    name: str
    tests: List[TestResult]
    duration: float
    setup_time: float = 0.0
    teardown_time: float = 0.0

class TestOrchestrator:
    """Main test orchestration class"""
    
    def __init__(self, project_root: Path, config: Dict):
        self.project_root = project_root
        self.config = config
        self.results: List[TestSuite] = []
        self.start_time = time.time()
        
        # Setup logging
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s [%(levelname)s] %(message)s',
            handlers=[
                logging.FileHandler(project_root / 'test_run.log'),
                logging.StreamHandler()
            ]
        )
        self.logger = logging.getLogger(__name__)
        
    def run_command(self, cmd: List[str], cwd: Optional[Path] = None, 
                   timeout: int = 300) -> Tuple[int, str, str]:
        """Run a command and return exit code, stdout, stderr"""
        try:
            process = subprocess.Popen(
                cmd,
                cwd=cwd or self.project_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            stdout, stderr = process.communicate(timeout=timeout)
            return process.returncode, stdout, stderr
            
        except subprocess.TimeoutExpired:
            process.kill()
            return -1, "", f"Command timed out after {timeout} seconds"
        except Exception as e:
            return -1, "", str(e)
    
    def run_unit_tests(self) -> TestSuite:
        """Run unit test suite"""
        self.logger.info("Running unit tests...")
        suite_start = time.time()
        tests = []
        
        unit_test_dir = self.project_root / "tests" / "unit"
        
        # Build unit tests
        build_start = time.time()
        ret_code, stdout, stderr = self.run_command(
            ["make", "clean", "all"], 
            cwd=unit_test_dir
        )
        build_time = time.time() - build_start
        
        if ret_code != 0:
            tests.append(TestResult(
                name="unit_tests_build",
                status="FAIL",
                duration=build_time,
                message="Failed to build unit tests",
                error_output=stderr
            ))
            return TestSuite("Unit Tests", tests, time.time() - suite_start)
        
        # Run unit test categories
        test_categories = [
            "test_core_functionality",
            "test_memory_management", 
            "test_tensor_operations",
            "test_performance_monitoring"
        ]
        
        for category in test_categories:
            test_start = time.time()
            ret_code, stdout, stderr = self.run_command(
                ["make", f"test-{category}"],
                cwd=unit_test_dir,
                timeout=120
            )
            test_duration = time.time() - test_start
            
            if ret_code == 0:
                status = "PASS"
                message = f"Unit test category {category} passed"
            else:
                status = "FAIL"
                message = f"Unit test category {category} failed"
            
            tests.append(TestResult(
                name=category,
                status=status,
                duration=test_duration,
                message=message,
                output=stdout,
                error_output=stderr
            ))
        
        # Run coverage analysis
        if self.config.get("enable_coverage", False):
            cov_start = time.time()
            ret_code, stdout, stderr = self.run_command(
                ["make", "coverage"],
                cwd=unit_test_dir
            )
            cov_duration = time.time() - cov_start
            
            tests.append(TestResult(
                name="coverage_analysis",
                status="PASS" if ret_code == 0 else "FAIL",
                duration=cov_duration,
                message="Coverage analysis completed",
                output=stdout,
                error_output=stderr
            ))
        
        return TestSuite("Unit Tests", tests, time.time() - suite_start)
    
    def run_integration_tests(self) -> TestSuite:
        """Run integration test suite"""
        self.logger.info("Running integration tests...")
        suite_start = time.time()
        tests = []
        
        integration_test_dir = self.project_root / "tests" / "integration"
        
        # Build integration tests
        build_start = time.time()
        ret_code, stdout, stderr = self.run_command(
            ["make", "clean", "all"],
            cwd=integration_test_dir
        )
        build_time = time.time() - build_start
        
        if ret_code != 0:
            tests.append(TestResult(
                name="integration_tests_build",
                status="FAIL",
                duration=build_time,
                message="Failed to build integration tests",
                error_output=stderr
            ))
            return TestSuite("Integration Tests", tests, time.time() - suite_start)
        
        # Run integration test suites
        test_suites = [
            ("e2e_tests", "End-to-end integration tests"),
            ("stress_tests", "Stress and reliability tests")
        ]
        
        for suite_name, description in test_suites:
            test_start = time.time()
            
            # Check if hardware is available
            if self.config.get("hardware_available", False):
                cmd = ["./integration_test_main", "--" + suite_name.replace("_", "-")]
            else:
                cmd = ["./integration_test_main", "--" + suite_name.replace("_", "-"), "--software-only"]
            
            ret_code, stdout, stderr = self.run_command(
                cmd,
                cwd=integration_test_dir,
                timeout=600  # 10 minutes for integration tests
            )
            test_duration = time.time() - test_start
            
            tests.append(TestResult(
                name=suite_name,
                status="PASS" if ret_code == 0 else "FAIL",
                duration=test_duration,
                message=description,
                output=stdout,
                error_output=stderr
            ))
        
        return TestSuite("Integration Tests", tests, time.time() - suite_start)
    
    def run_simulation_tests(self) -> TestSuite:
        """Run hardware simulation tests"""
        self.logger.info("Running simulation tests...")
        suite_start = time.time()
        tests = []
        
        simulation_test_dir = self.project_root / "tests" / "simulation"
        
        # Check if simulation tools are available
        sim_tools = ["vivado", "vsim", "vcs"]
        available_sim = None
        
        for tool in sim_tools:
            if shutil.which(tool):
                available_sim = tool
                break
        
        if not available_sim:
            tests.append(TestResult(
                name="simulation_tools_check",
                status="SKIP",
                duration=0.0,
                message="No simulation tools available"
            ))
            return TestSuite("Simulation Tests", tests, time.time() - suite_start)
        
        self.logger.info(f"Using simulation tool: {available_sim}")
        
        # Run simulation test categories
        sim_tests = [
            ("test-quick", "Quick functionality tests"),
            ("test-full", "Comprehensive simulation tests") if self.config.get("run_full_sim", False) else None
        ]
        
        sim_tests = [test for test in sim_tests if test is not None]
        
        for test_target, description in sim_tests:
            test_start = time.time()
            ret_code, stdout, stderr = self.run_command(
                ["make", test_target, f"SIM={available_sim}"],
                cwd=simulation_test_dir,
                timeout=1800  # 30 minutes for simulation
            )
            test_duration = time.time() - test_start
            
            tests.append(TestResult(
                name=test_target,
                status="PASS" if ret_code == 0 else "FAIL",
                duration=test_duration,
                message=description,
                output=stdout,
                error_output=stderr
            ))
        
        return TestSuite("Simulation Tests", tests, time.time() - suite_start)
    
    def run_performance_tests(self) -> TestSuite:
        """Run performance benchmark tests"""
        self.logger.info("Running performance tests...")
        suite_start = time.time()
        tests = []
        
        benchmark_dir = self.project_root / "tests" / "benchmarks"
        
        # Build benchmarks
        build_start = time.time()
        ret_code, stdout, stderr = self.run_command(
            ["make", "clean", "all"],
            cwd=benchmark_dir
        )
        build_time = time.time() - build_start
        
        if ret_code != 0:
            tests.append(TestResult(
                name="benchmark_build",
                status="FAIL",
                duration=build_time,
                message="Failed to build benchmarks",
                error_output=stderr
            ))
            return TestSuite("Performance Tests", tests, time.time() - suite_start)
        
        # Run performance benchmarks
        benchmark_tests = [
            ("run-quick", "Quick performance validation"),
        ]
        
        if self.config.get("run_full_benchmarks", False):
            benchmark_tests.extend([
                ("run-throughput", "Throughput benchmarks"),
                ("run-latency", "Latency benchmarks"),
                ("run-scalability", "Scalability benchmarks")
            ])
        
        for test_target, description in benchmark_tests:
            test_start = time.time()
            ret_code, stdout, stderr = self.run_command(
                ["make", test_target],
                cwd=benchmark_dir,
                timeout=900  # 15 minutes for benchmarks
            )
            test_duration = time.time() - test_start
            
            tests.append(TestResult(
                name=test_target,
                status="PASS" if ret_code == 0 else "FAIL",
                duration=test_duration,
                message=description,
                output=stdout,
                error_output=stderr
            ))
        
        return TestSuite("Performance Tests", tests, time.time() - suite_start)
    
    def run_static_analysis(self) -> TestSuite:
        """Run static analysis checks"""
        self.logger.info("Running static analysis...")
        suite_start = time.time()
        tests = []
        
        # Cppcheck analysis
        test_start = time.time()
        ret_code, stdout, stderr = self.run_command([
            "cppcheck",
            "--enable=all",
            "--inconclusive",
            "--error-exitcode=1",
            "--suppress=missingIncludeSystem",
            "software/", "tests/"
        ])
        test_duration = time.time() - test_start
        
        tests.append(TestResult(
            name="cppcheck_analysis",
            status="PASS" if ret_code == 0 else "FAIL",
            duration=test_duration,
            message="Static analysis with cppcheck",
            output=stdout,
            error_output=stderr
        ))
        
        # Clang-tidy analysis (if available)
        if shutil.which("clang-tidy"):
            test_start = time.time()
            ret_code, stdout, stderr = self.run_command([
                "find", "software/", "-name", "*.c",
                "-exec", "clang-tidy", "{}", "--"
            ])
            test_duration = time.time() - test_start
            
            tests.append(TestResult(
                name="clang_tidy_analysis",
                status="PASS" if ret_code == 0 else "FAIL",
                duration=test_duration,
                message="Static analysis with clang-tidy",
                output=stdout,
                error_output=stderr
            ))
        
        return TestSuite("Static Analysis", tests, time.time() - suite_start)
    
    def generate_reports(self):
        """Generate test reports in multiple formats"""
        self.logger.info("Generating test reports...")
        
        report_dir = self.project_root / "test_reports"
        report_dir.mkdir(exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Generate JSON report
        json_report = {
            "timestamp": datetime.now().isoformat(),
            "total_duration": time.time() - self.start_time,
            "project_root": str(self.project_root),
            "config": self.config,
            "test_suites": []
        }
        
        total_tests = 0
        passed_tests = 0
        failed_tests = 0
        skipped_tests = 0
        
        for suite in self.results:
            suite_data = {
                "name": suite.name,
                "duration": suite.duration,
                "setup_time": suite.setup_time,
                "teardown_time": suite.teardown_time,
                "tests": []
            }
            
            for test in suite.tests:
                suite_data["tests"].append({
                    "name": test.name,
                    "status": test.status,
                    "duration": test.duration,
                    "message": test.message,
                    "output": test.output[:1000] if test.output else "",  # Truncate long output
                    "error_output": test.error_output[:1000] if test.error_output else ""
                })
                
                total_tests += 1
                if test.status == "PASS":
                    passed_tests += 1
                elif test.status == "FAIL" or test.status == "ERROR":
                    failed_tests += 1
                elif test.status == "SKIP":
                    skipped_tests += 1
            
            json_report["test_suites"].append(suite_data)
        
        json_report["summary"] = {
            "total_tests": total_tests,
            "passed_tests": passed_tests,
            "failed_tests": failed_tests,
            "skipped_tests": skipped_tests,
            "success_rate": (passed_tests / total_tests * 100) if total_tests > 0 else 0
        }
        
        # Save JSON report
        json_file = report_dir / f"test_report_{timestamp}.json"
        with open(json_file, 'w') as f:
            json.dump(json_report, f, indent=2)
        
        # Generate HTML report
        html_content = self.generate_html_report(json_report)
        html_file = report_dir / f"test_report_{timestamp}.html"
        with open(html_file, 'w') as f:
            f.write(html_content)
        
        # Generate JUnit XML report for CI integration
        junit_content = self.generate_junit_xml(json_report)
        junit_file = report_dir / f"junit_report_{timestamp}.xml"
        with open(junit_file, 'w') as f:
            f.write(junit_content)
        
        self.logger.info(f"Reports generated:")
        self.logger.info(f"  JSON: {json_file}")
        self.logger.info(f"  HTML: {html_file}")
        self.logger.info(f"  JUnit XML: {junit_file}")
        
        return json_report
    
    def generate_html_report(self, report_data: Dict) -> str:
        """Generate HTML test report"""
        summary = report_data["summary"]
        
        html = f"""
<!DOCTYPE html>
<html>
<head>
    <title>FPGA NPU Test Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        .header {{ background-color: #f0f0f0; padding: 20px; border-radius: 5px; }}
        .summary {{ display: flex; gap: 20px; margin: 20px 0; }}
        .metric {{ background-color: #e9ecef; padding: 15px; border-radius: 5px; text-align: center; }}
        .suite {{ margin: 20px 0; border: 1px solid #ddd; border-radius: 5px; }}
        .suite-header {{ background-color: #f8f9fa; padding: 15px; font-weight: bold; }}
        .test {{ padding: 10px; border-bottom: 1px solid #eee; }}
        .test:last-child {{ border-bottom: none; }}
        .status-PASS {{ color: #28a745; }}
        .status-FAIL {{ color: #dc3545; }}
        .status-SKIP {{ color: #6c757d; }}
        .status-ERROR {{ color: #fd7e14; }}
        .output {{ background-color: #f8f9fa; padding: 10px; margin: 10px 0; font-family: monospace; white-space: pre-wrap; }}
    </style>
</head>
<body>
    <div class="header">
        <h1>FPGA NPU Test Report</h1>
        <p>Generated: {report_data["timestamp"]}</p>
        <p>Duration: {report_data["total_duration"]:.2f} seconds</p>
    </div>
    
    <div class="summary">
        <div class="metric">
            <h3>{summary["total_tests"]}</h3>
            <p>Total Tests</p>
        </div>
        <div class="metric">
            <h3 class="status-PASS">{summary["passed_tests"]}</h3>
            <p>Passed</p>
        </div>
        <div class="metric">
            <h3 class="status-FAIL">{summary["failed_tests"]}</h3>
            <p>Failed</p>
        </div>
        <div class="metric">
            <h3 class="status-SKIP">{summary["skipped_tests"]}</h3>
            <p>Skipped</p>
        </div>
        <div class="metric">
            <h3>{summary["success_rate"]:.1f}%</h3>
            <p>Success Rate</p>
        </div>
    </div>
"""
        
        for suite in report_data["test_suites"]:
            html += f"""
    <div class="suite">
        <div class="suite-header">
            {suite["name"]} ({suite["duration"]:.2f}s)
        </div>
"""
            for test in suite["tests"]:
                html += f"""
        <div class="test">
            <strong class="status-{test["status"]}">[{test["status"]}]</strong>
            {test["name"]} ({test["duration"]:.2f}s)
            <br>
            <em>{test["message"]}</em>
"""
                if test["error_output"]:
                    html += f'<div class="output">Error: {test["error_output"]}</div>'
                
                html += "</div>"
            
            html += "</div>"
        
        html += """
</body>
</html>
"""
        return html
    
    def generate_junit_xml(self, report_data: Dict) -> str:
        """Generate JUnit XML report for CI systems"""
        xml = '<?xml version="1.0" encoding="UTF-8"?>\n'
        xml += f'<testsuites tests="{report_data["summary"]["total_tests"]}" '
        xml += f'failures="{report_data["summary"]["failed_tests"]}" '
        xml += f'skipped="{report_data["summary"]["skipped_tests"]}" '
        xml += f'time="{report_data["total_duration"]:.3f}">\n'
        
        for suite in report_data["test_suites"]:
            suite_tests = len(suite["tests"])
            suite_failures = sum(1 for test in suite["tests"] if test["status"] in ["FAIL", "ERROR"])
            suite_skipped = sum(1 for test in suite["tests"] if test["status"] == "SKIP")
            
            xml += f'  <testsuite name="{suite["name"]}" '
            xml += f'tests="{suite_tests}" '
            xml += f'failures="{suite_failures}" '
            xml += f'skipped="{suite_skipped}" '
            xml += f'time="{suite["duration"]:.3f}">\n'
            
            for test in suite["tests"]:
                xml += f'    <testcase name="{test["name"]}" time="{test["duration"]:.3f}"'
                
                if test["status"] == "PASS":
                    xml += '/>\n'
                elif test["status"] in ["FAIL", "ERROR"]:
                    xml += '>\n'
                    xml += f'      <failure message="{test["message"]}">'
                    xml += f'{test["error_output"]}</failure>\n'
                    xml += '    </testcase>\n'
                elif test["status"] == "SKIP":
                    xml += '>\n'
                    xml += f'      <skipped message="{test["message"]}"/>\n'
                    xml += '    </testcase>\n'
            
            xml += '  </testsuite>\n'
        
        xml += '</testsuites>\n'
        return xml
    
    def run_all_tests(self):
        """Run all test suites"""
        self.logger.info("Starting comprehensive test run...")
        
        # Run test suites based on configuration
        test_suites = []
        
        if self.config.get("run_unit_tests", True):
            test_suites.append(("unit", self.run_unit_tests))
        
        if self.config.get("run_static_analysis", True):
            test_suites.append(("static", self.run_static_analysis))
        
        if self.config.get("run_integration_tests", True):
            test_suites.append(("integration", self.run_integration_tests))
        
        if self.config.get("run_simulation_tests", False):
            test_suites.append(("simulation", self.run_simulation_tests))
        
        if self.config.get("run_performance_tests", False):
            test_suites.append(("performance", self.run_performance_tests))
        
        # Run test suites in parallel or sequential based on config
        if self.config.get("parallel_execution", False):
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
                future_to_suite = {executor.submit(func): name for name, func in test_suites}
                
                for future in concurrent.futures.as_completed(future_to_suite):
                    suite_name = future_to_suite[future]
                    try:
                        result = future.result()
                        self.results.append(result)
                        self.logger.info(f"Completed test suite: {suite_name}")
                    except Exception as exc:
                        self.logger.error(f"Test suite {suite_name} generated an exception: {exc}")
        else:
            for name, func in test_suites:
                try:
                    result = func()
                    self.results.append(result)
                    self.logger.info(f"Completed test suite: {name}")
                except Exception as exc:
                    self.logger.error(f"Test suite {name} failed with exception: {exc}")
        
        # Generate reports
        report = self.generate_reports()
        
        # Print summary
        self.print_summary(report)
        
        return report["summary"]["failed_tests"] == 0
    
    def print_summary(self, report: Dict):
        """Print test run summary"""
        summary = report["summary"]
        
        print("\n" + "="*60)
        print("TEST RUN SUMMARY")
        print("="*60)
        print(f"Total Tests:    {summary['total_tests']}")
        print(f"Passed:         {summary['passed_tests']}")
        print(f"Failed:         {summary['failed_tests']}")
        print(f"Skipped:        {summary['skipped_tests']}")
        print(f"Success Rate:   {summary['success_rate']:.1f}%")
        print(f"Duration:       {report['total_duration']:.2f} seconds")
        print("="*60)
        
        if summary['failed_tests'] > 0:
            print("❌ TEST RUN FAILED")
            sys.exit(1)
        else:
            print("✅ TEST RUN PASSED")

def main():
    parser = argparse.ArgumentParser(description="FPGA NPU Automated Testing Framework")
    parser.add_argument("--project-root", type=Path, default=Path.cwd(),
                       help="Project root directory")
    parser.add_argument("--config", type=Path,
                       help="Configuration file path")
    parser.add_argument("--unit-tests", action="store_true", default=True,
                       help="Run unit tests")
    parser.add_argument("--integration-tests", action="store_true", default=True,
                       help="Run integration tests")
    parser.add_argument("--simulation-tests", action="store_true",
                       help="Run simulation tests")
    parser.add_argument("--performance-tests", action="store_true",
                       help="Run performance tests")
    parser.add_argument("--static-analysis", action="store_true", default=True,
                       help="Run static analysis")
    parser.add_argument("--parallel", action="store_true",
                       help="Run test suites in parallel")
    parser.add_argument("--hardware-available", action="store_true",
                       help="Hardware is available for testing")
    parser.add_argument("--enable-coverage", action="store_true",
                       help="Enable code coverage analysis")
    parser.add_argument("--verbose", "-v", action="store_true",
                       help="Verbose output")
    
    args = parser.parse_args()
    
    # Load configuration
    config = {
        "run_unit_tests": args.unit_tests,
        "run_integration_tests": args.integration_tests,
        "run_simulation_tests": args.simulation_tests,
        "run_performance_tests": args.performance_tests,
        "run_static_analysis": args.static_analysis,
        "parallel_execution": args.parallel,
        "hardware_available": args.hardware_available,
        "enable_coverage": args.enable_coverage,
        "run_full_sim": False,
        "run_full_benchmarks": False
    }
    
    if args.config and args.config.exists():
        with open(args.config) as f:
            file_config = json.load(f)
            config.update(file_config)
    
    # Setup verbose logging
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Run tests
    orchestrator = TestOrchestrator(args.project_root, config)
    success = orchestrator.run_all_tests()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()