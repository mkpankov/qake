import subprocess as sp

i = 0
code = False
code_lines = dict()
file_names = []

for l in open('Makefile.md').read().split('\n'):
    if l.startswith('```') and not code:
        code = True
        language = l[3:]
        continue
    if code and l.startswith('```'):
        file_name = 'render{0}.mk'.format(i)
        file_names.append(file_name)
        open(file_name, 'w').write('\n'.join(code_lines[i]))
        i += 1
        continue
    if code:
        if i not in code_lines.keys():
            code_lines[i] = []
        code_lines[i].append(l[4:])

for f in file_names:
    sp.check_call('pygmentize -l makefile -o {0}.html {0}'.format(f), shell=True)
