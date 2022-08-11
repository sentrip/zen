import sys
from pathlib import Path


def parse_path(path: Path, dependencies: dict):
    depends = []
    for line in path.read_text().splitlines():
        if line.startswith('#include'):
            for open_char, close_char in [('"', '"'), ('<', '>')]:
                begin = line.find(open_char)
                if begin != -1:
                    end = line.find(close_char, begin + 1)
                    include = line[begin+1:end]
                    depends.append(include if open_char == '"' else f'<{include}>')
                    if open_char != '<':                        
                        parse_path(path.parent / include, dependencies)

    dependencies[path.name] = depends


def expand_dependencies(dependencies: dict) -> dict:
    expanded = {}
    for k in dependencies:
        n = -1
        ex = set(dependencies[k])
        while n != len(ex):
            for d in list(ex):
                rest = dependencies.get(d, [])
                if rest:
                    ex.update(rest)
            n = len(ex)
        expanded[k] = ex
    return expanded


def print_expanded_dependencies(expanded: dict, only_user: bool):
    final = sorted([(k, sorted(list(v))) for k, v in expanded.items()], key=lambda x: len(x[1]))
    for k, dep in final:
        print(f'{k}')
        indent = 4
        n_std = sum(1 for x in dep if x.startswith('<'))

        if not only_user:
            print(f'{" " * indent}std:')
            indent += 4
            for d in dep[:n_std]:
                print(f'{" " * indent}{d}')
            indent -= 4

        if n_std < len(dep):
            if not only_user:
                print(f'    user:')
                indent += 4
            for d in dep[n_std:]:
                print(f'{" " * indent}{d}')
            if not only_user:
                indent -= 4

        print('')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise RuntimeError('Usage: python depedencies.py FOLDER [-u]')
    
    root = Path(sys.argv[1])
    only_user_headers = len(sys.argv) > 2 and sys.argv[2] == '-u'
    if not root.exists():
        raise RuntimeError(f'{root} does not exist')
    
    if not root.is_dir():
        raise RuntimeError(f'{root} is not a directory')

    dependencies = {}
    for path in root.iterdir():
        if not path.is_dir():
            parse_path(path, dependencies)

    print_expanded_dependencies(expand_dependencies(dependencies), only_user_headers)
