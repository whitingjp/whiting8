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

cflags = '-Isrc -Iinput/glfw/include -Iinput/glew/include -Wall -Wextra -Werror'
ldflags = ' -Linput/glfw/lib-mingw -Linput/glew -lglew32s -lglfw -lglu32 -lopengl32'
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

inputdir = 'input'
builddir = 'build'
outdir = joinp(builddir, 'whiting8')
testdir = joinp(builddir, 'test')
data_in = 'data'
data_out = joinp(outdir, 'data')

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
n.rule('test',
  command='$in --test $out',
  description='TEST $in')
n.rule('assemble',
  command='%s $in $out' % joinp(outdir, 'assembler'),
  description='ASSEMBLE $in $out')
n.newline()

objdir = joinp(builddir, 'obj')
def obj_walk(srcdir):
  obj = []
  for (dirpath, dirnames, filenames) in os.walk(srcdir):
    for f in filenames:
      _, ext = os.path.splitext(f)
      if ext == '.c':
        s = os.path.relpath(joinp(dirpath, f), srcdir)
        o = s.replace('.c', '.o')
        obj += n.build(joinp(objdir, o), 'cxx', joinp(srcdir, s))
  return obj

commondir = joinp('src', 'common')
commonobj = obj_walk(commondir)

names = ['assembler','emulator']
targets = []
tests = []
for name in names:
  target = name+'.exe'
  namedir = joinp('src', name)
  obj = obj_walk(namedir) + commonobj
  targets += n.build(joinp(outdir, target), 'link', obj)
  tests += n.build(joinp(testdir, name+'.log'), 'test', joinp(outdir, target))
  n.newline()
apps = n.build('apps', 'phony', targets)
test = n.build('test', 'phony', apps+tests)
n.newline()

data = []
data += n.build(joinp(outdir, 'readme.txt'), 'cp', 'README.md')
targets += n.build('data', 'phony', data)
n.newline()

programsrc = joinp('src', 'programs')
programout = joinp(outdir, 'programs')
programs = []
for (dirpath, dirnames, filenames) in os.walk(programsrc):
  for f in filenames:
    _, ext = os.path.splitext(f)
    if ext == '.wta':
      s = os.path.relpath(joinp(dirpath, f), programsrc)
      o = s.replace('.wta', '.wt8')
      programs += n.build(joinp(programout, o), 'assemble', [joinp(programsrc, s)], apps)
progs = n.build('progs', 'phony', programs)
n.newline()

all = n.build('all', 'phony', apps+data+test+progs)
n.default('all')
