
# A script that prints out some basic stats
import glob
import sys, os

nHeaderCount = 0
nSourceCount = 0
nHeaderLineCount = 0
nSourceLineCount = 0
nHeaderSize  = 0
nSourceSize  = 0

def GetLineCount(file):
    with open(file, 'r') as f:
        return sum([1 for _ in f.readlines()])

def BytesToString(n):
    if n > 1024 ** 3: # GiB
        return f'{round(n / 1024 ** 3, 2)}GiB'
    elif n > 1024 ** 2: # MiB
        return f'{round(n / 1024 ** 2, 2)}MiB'
    elif n >= 1024: # KiB
        return f'{round(n / 1024 ** 1, 2)}KiB'
    return f'{n}B'

for file in glob.glob(os.getcwd() + '/**', recursive=True):
    if os.path.islink(os.path.dirname(file)):
        continue
    if os.path.splitext(file)[1] == '.c':
        nSourceCount += 1
        nSourceSize += os.path.getsize(file)
        nSourceLineCount += GetLineCount(file)
    elif os.path.splitext(file)[1] == '.h':
        nHeaderCount += 1
        nHeaderSize += os.path.getsize(file)
        nHeaderLineCount += GetLineCount(file)

print(f'Number of headers: {nHeaderCount}')
print(f'Number of sources: {nSourceCount}')
print(f'Total size of headers: {BytesToString(nHeaderSize)}')
print(f'Total size of sources: {BytesToString(nSourceSize)}')
print(f'Total line count of headers: {nHeaderLineCount}')
print(f'Total line count of sources: {nSourceLineCount}')
print(f'Project size: {BytesToString(nHeaderSize + nSourceSize)}')
print(f'Project line count: {nHeaderLineCount + nSourceLineCount}')
