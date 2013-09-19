import sys
sys.path.insert(0, 'input')
import ninja_syntax
import os
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--profile', action='store_true')
parser.add_argument('--debug', action='store_true')
args = parser.parse_args()

joinp = os.path.join

BUILD_FILENAME = 'build.ninja'
buildfile = open(BUILD_FILENAME, 'w')
n = ninja_syntax.Writer(buildfile)

objext = '.o'

cflags = '-Isrc -Iinput/glfw/include -Wall -Wextra -Werror'
ldflags = ' -Linput/glfw/lib-mingw -lglfw -lglu32 -lopengl32'
if(args.debug):
  cflags = cflags + ' -g'
else:
  cflags = cflags + ' -O3'
if(args.profile):
  cflags = cflags + ' -pg'
  ldflags = ldflags + ' -pg'

n.variable('cflags', cflags)
n.variable('ldflags', ldflags)
n.newline()

n.rule('cxx',
  command='gcc -MMD -MF $out.d $cflags -c $in -o $out',
  depfile='$out.d',
  description='CXX $out')
n.rule('link',
  command='gcc $in $libs $ldflags -o $out',
  description='LINK $out')
n.rule('cp',
  command='cp $in $out',
  description='COPY $in $out')
n.newline()

inputdir = 'input'
builddir = 'build'
outdir = joinp(builddir, 'whiting8')
data_in = 'data'
data_out = joinp(outdir, 'data')

names = ['emulator']
for name in names:
  target = name+'.exe'
  srcdir = joinp('src', name)
  objdir = joinp(builddir, 'obj')
  targets = []
  obj = []
  for (dirpath, dirnames, filenames) in os.walk(srcdir):
    for f in filenames:
      _, ext = os.path.splitext(f)
      if ext == '.c':
        s = os.path.relpath(joinp(dirpath, f), srcdir)
        o = s.replace('.c', '.o')
        obj += n.build(joinp(objdir, o), 'cxx', joinp(srcdir, s))
  targets += n.build(joinp(outdir, target), 'link', obj)
  n.newline()

data = []
for (dirpath, dirnames, filenames) in os.walk(data_in):
  for f in filenames:
    s = os.path.relpath(joinp(dirpath, f), data_in)
    data += n.build(joinp(data_out, s), 'cp', joinp(data_in, s))    
n.newline()

data += n.build(joinp(outdir, 'readme.txt'), 'cp', 'README.md')


targets += n.build('data', 'phony', data)
n.newline()

n.build('all', 'phony', targets)
n.default('all')