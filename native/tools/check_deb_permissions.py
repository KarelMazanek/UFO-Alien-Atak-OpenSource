"""Reject DEBs whose root-owned payload cannot be accessed by normal users."""
import argparse
import subprocess
import tarfile

parser = argparse.ArgumentParser()
parser.add_argument('packages', nargs='+')
args = parser.parse_args()
for package in args.packages:
    failures = []
    directories = 0
    process = subprocess.Popen(['dpkg-deb', '--fsys-tarfile', package], stdout=subprocess.PIPE)
    with tarfile.open(fileobj=process.stdout, mode='r|') as archive:
        for member in archive:
            if member.isdir():
                directories += 1
                if member.mode & 0o777 != 0o755:
                    failures.append(f'{member.name}: directory mode {member.mode:o}, expected 755')
            elif member.isfile():
                if not member.mode & 0o004:
                    failures.append(f'{member.name}: not readable by ordinary users')
                if member.name.endswith(('/uaa-native', '/uaa-mapeditor', '/bin/uaa-native-dev', '/bin/uaa-native-mapeditor')):
                    if member.mode & 0o111 != 0o111:
                        failures.append(f'{member.name}: not executable by ordinary users')
    process.stdout.close()
    if process.wait():
        raise SystemExit('dpkg-deb could not read ' + package)
    if failures:
        raise SystemExit('\n'.join(failures))
    print(f'PASS: {package}: {directories} directories 0755; files readable; launchers executable')
