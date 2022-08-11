import sys
from pathlib import Path

SRC_DIR = Path(__file__).parent.parent / 'src'

PACKS = {
    'core': ['zen_array.h', 'zen_bitset.h', 'zen_span.h', 'zen_fmt.h'],
    'all': [f.name for f in SRC_DIR.iterdir() if f.name != 'zen.h']
}


def parse_path(path: Path, dependencies: dict, data: dict):
    depends = []
    file = path.read_text()
    data[path.name] = file
    for line in file.splitlines():
        if line.startswith('#include'):
            begin = line.find('"')
            if begin != -1:
                end = line.find('"', begin + 1)
                include = line[begin+1:end]
                depends.append(include)
                parse_path(path.parent / include, dependencies, data)
    dependencies[path.name] = depends


if __name__ == '__main__':
    if len(sys.argv) < 3:
        raise RuntimeError('Usage: python single_header.py FOLDER NAMES...')
    
    root = SRC_DIR
    names = sys.argv[1:]
    output = root / 'single_header.h'
    if '-o' in names:
        i = names.index('-o')
        if i == len(names) - 1:
            raise RuntimeError('Did not provide output file path')
        output = Path(names[i + 1])
        names = names[:i] + names[i + 2:]
    
    data = {}
    dependencies = {}
    for name in names:
        if name in PACKS:
            for n in PACKS[name]:
                parse_path(root / n, dependencies, data)
        else:
            parse_path(root / name, dependencies, data)

    seen = set()
    sorted_dependencies = []
    while len(sorted_dependencies) != len(dependencies):
        for k, v in dependencies.items():
            if k not in seen:
                if not v:
                    sorted_dependencies.append(k)
                    seen.add(k)
                elif all(x in seen for x in v):
                    sorted_dependencies.append(k)
                    seen.add(k)
    
    seen_std_headers = set()
    with open(output, 'w') as f:
        for name in sorted_dependencies:
            for line in data[name].splitlines(True):
                if line.startswith("#include"):
                    begin = line.find('<')
                    if begin == -1:
                        continue
                    end = line.find('>', begin + 1)
                    include = line[begin + 1:end]
                    if include not in seen_std_headers:
                        seen_std_headers.add(include)
                        f.write(line)
                else:
                    f.write(line)
            f.write('\n')
