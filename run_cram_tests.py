import difflib
import os
import subprocess
import sys

BASE_DIR = "tests"
TESTS_DIR = os.path.join(BASE_DIR, "examples")
EXPECTED_C_DIR = os.path.join(BASE_DIR, "expected", "c_output")
EXPECTED_STDOUT_DIR = os.path.join(BASE_DIR, "expected", "stdout")

BUILD_SCRIPT = "./build.sh"
ALLOCATOR = "mark_sweep"
UPDATE_MODE = False


def read_file(path):
    if not os.path.exists(path):
        return []
    with open(path, "r", encoding="utf-8") as f:
        return f.read().splitlines()


def write_file(path, lines):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def run_test(test_file, default_allocator):
    print(f"\n=== Running test: {test_file} ===")
    allocator = "reference_count" if "ref_count" in test_file else default_allocator

    test_name = os.path.splitext(os.path.basename(test_file))[0]
    expected_c_path = os.path.join(EXPECTED_C_DIR, f"{test_name}.c")
    expected_out_path = os.path.join(EXPECTED_STDOUT_DIR, f"{test_name}.out")

    try:
        subprocess.run(
            [BUILD_SCRIPT, test_file, allocator, "--build-only", "--debug"],
            capture_output=True,
            text=True,
            check=True
        )
    except subprocess.CalledProcessError as e:
        print(f"[FAIL] Build failed for {test_file}")
        print(e.stderr)
        return False

    actual_c = read_file("output.c")

    try:
        result = subprocess.run(
            ["./output"],
            capture_output=True,
            text=True,
            check=True
        )
        actual_out = result.stdout.splitlines()
    except subprocess.CalledProcessError as e:
        print(f"[FAIL] Runtime error in {test_name}")
        print(e.stderr)
        return False

    if UPDATE_MODE:
        write_file(expected_c_path, actual_c)
        write_file(expected_out_path, actual_out)
        print(f"[PASS] Updated expected outputs for {test_name}")
        return True

    expected_c = read_file(expected_c_path)
    expected_out = read_file(expected_out_path)

    c_match = actual_c == expected_c
    out_match = actual_out == expected_out

    if not c_match:
        print(f"[FAIL] Transpiled C code does not match for {test_name}")
        print("--- Diff (C code) ---")
        for line in difflib.unified_diff(expected_c, actual_c, fromfile='expected', tofile='actual'):
            print(line)

    if not out_match:
        print(f"[FAIL] Stdout does not match for {test_name}")
        print("--- Diff (stdout) ---")
        for line in difflib.unified_diff(expected_out, actual_out, fromfile='expected', tofile='actual'):
            print(line)

    if c_match and out_match:
        print(f"[PASS] Test {test_name} passed!")

    return c_match and out_match


def main():
    global ALLOCATOR, UPDATE_MODE
    test_name = None

    for arg in sys.argv[1:]:
        if arg in ["--update", "-u"]:
            UPDATE_MODE = True
        else:
            test_name = arg

    if test_name:
        test_files = [os.path.join(TESTS_DIR, f)
                      for f in os.listdir(TESTS_DIR)
                      if f.endswith(".jb") and f.startswith(test_name)]
    else:
        test_files = [os.path.join(TESTS_DIR, f)
                      for f in os.listdir(TESTS_DIR)
                      if f.endswith(".jb")]

    passed = 0
    for test_file in sorted(test_files):
        if run_test(test_file, ALLOCATOR):
            passed += 1

    print(f"\nSummary: {passed}/{len(test_files)} tests {'updated' if UPDATE_MODE else 'passed'}.")


if __name__ == "__main__":
    main()
