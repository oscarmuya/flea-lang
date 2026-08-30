import os
import subprocess
import sys


def run_tests():
    test_dir = "tests"
    failed = False

    for file in sorted(os.listdir(test_dir)):
        if not file.endswith(".fl"):
            continue

        filepath = os.path.join(test_dir, file)

        # Collect every EXPECTED value in the order it appears in the file
        expected_outputs = []
        with open(filepath, "r") as f:
            for line in f:
                if "EXPECTED:" in line:
                    expected_outputs.append(line.split("EXPECTED:")[1].strip())

        if not expected_outputs:
            continue

        # Run the compiled binary once against the whole file
        result = subprocess.run(["./flea", filepath], capture_output=True, text=True)

        # Each print statement is expected to produce one line of stdout
        actual_outputs = [line.strip() for line in result.stdout.strip().splitlines()]

        if len(actual_outputs) != len(expected_outputs):
            print(f"❌ {file}: Failed!")
            print(f"   Expected {len(expected_outputs)} output line(s), got {len(actual_outputs)}")
            print(f"   Expected: {expected_outputs}")
            print(f"   Got:      {actual_outputs}")
            if result.stderr:
                print(f"   Stderr:   {result.stderr.strip()}")
            failed = True
            continue

        file_failed = False
        for i, (expected, actual) in enumerate(zip(expected_outputs, actual_outputs), start=1):
            if actual != expected:
                if not file_failed:
                    print(f"❌ {file}: Failed!")
                    file_failed = True
                print(f"   Check {i}: Expected '{expected}', Got '{actual}'")

        if file_failed:
            failed = True
        else:
            print(f"✅ {file}: Passed ({len(expected_outputs)} check(s))")

    if failed:
        sys.exit(1)


if __name__ == "__main__":
    run_tests()
