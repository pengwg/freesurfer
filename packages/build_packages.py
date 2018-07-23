#!/usr/bin/env python

# This is a utility script to help external developers easily build
# freesurfer dependencies. The packages, defined below in the 'pkgs' list,
# are built with the scripts and tarballs stored in the packages/source dir
# and will get installed to the destination directory specified on the command-line.
# 
# usage:
#    build_packages.py /path/to/install/destination
#
# Particular packages can be ignored with the --no-<package-name> flags documented in the
# help text. This script loops through each package object defined below, extracts the
# assocated tarball to <destination-dir>/<package-name>/<version>/src, and runs
# the provided build script. The individual build scripts should always expect a single
# input argument that specifies the package install directory. After each successful package
# build, the checksum of the tarball + build script is saved to the src dir. This way,
# if this script is rerun, the package is only rebuilt when the source code has been
# modified/updated.

import os
import sys
import shutil
import argparse
import tarfile
import subprocess
import hashlib

# get the absolute path of this build_packages.py script, so we can
# locate the source tarballs and scripts (and import the log module while we're at it)
fs_source_dir = os.path.dirname(os.path.abspath(os.path.dirname(sys.argv[0])))
sys.path.append(os.path.join(fs_source_dir, 'python'))
from freesurfer.log import *

# simple package class to store package source information
class Package:
  def __init__(self, name, version, script, tarball, required=True):
    self.name = name
    self.version = version
    self.required = required
    self.script = os.path.join(fs_source_dir, 'packages/source', script)
    if not os.path.exists(self.script):
      errorExit('%s does not exist' % self.script)
    self.tarball = os.path.join(fs_source_dir, 'packages/source', tarball)
    if not os.path.exists(self.tarball):
      errorExit('%s does not exist' % self.tarball)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#                              ~~ freesurfer dependencies ~~

pkgs = [
  Package('jpeg',        '6b',     'build_jpeg.sh',   'jpeg-6b.tar.gz'),
  Package('tiff',        '3.6.1',  'build_tiff.sh',   'tiff-3.6.1.tar.gz'),
  Package('expat',       '2.0.1',  'build_expat.sh',  'expat-2.0.1.tar.gz'),
  Package('xml2',        '2.7.7',  'build_xml2.sh',   'xml2-2.7.7.tar.gz'),
  Package('glut',        '3.7',    'build_glut.sh',   'glut-3.7.tar.gz'),
  Package('netcdf',      '3.6.0',  'build_netcdf.sh', 'netcdf-3.6.0-p1.tar.gz'),
  Package('minc',        '1.5',    'build_minc.sh',   'minc-1.5.tar.gz'),  # it's important that minc is built after netcdf  
  Package('tetgen',      '1.4.1',  'build_tetgen.sh', 'tetgen-1.4.1.tar.gz'),
  Package('petsc',       '2.3.3',  'build_petsc.sh',  'petsc-2.3.3.tar.gz', required=False),
  Package('ann',         '1.1.2',  'build_ann.sh',    'ann-1.1.2.tar.gz', required=False),
  Package('tcltktixblt', '8.4.6',  'build_tcl.sh',    'tcltktixblt-8.4.6.tar.gz', required=False),
  Package('vtk',         '5.10.1', 'build_vtk.sh',    'vtk-5.10.1.tar.gz', required=False),
  Package('itk',         '5.0.0',  'build_itk.sh',    'itk-5.0.0.tar.gz')
]

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# parse the command line inputs
parser = argparse.ArgumentParser()
parser.add_argument('destination', help="installation dir for the packages")
parser.add_argument('-f', '--force', action='store_true', help="force rebuild")
for package in pkgs:
  # add option to skip individual packages
  parser.add_argument('--no-%s' % package.name, action='store_true', help="don't build %s %s" % (package.name, package.version))
args = parser.parse_args()  # parse the cmd line

# create the packages destination dir
destination_dir = os.path.abspath(args.destination)
if not os.path.exists(destination_dir): os.makedirs(destination_dir)

for package in pkgs:
  # cd into packages install destination
  os.chdir(destination_dir)
  
  # make sure we aren't skipping this one
  if vars(args)['no_%s' % package.name]:
    print('skipping %s%s %s%s...' % (term.bold, package.name, package.version, term.end))
    continue
  
  # get the actual package install dir
  package_dir = os.path.join(destination_dir, package.name, package.version)
  src_dir = os.path.join(package_dir, 'src')

  # compute the new MD5 of the tarball + build script
  md5 = hashlib.md5()
  f = open(package.tarball, 'rb')
  while True:
    tardata = f.read(2**20)
    if not tardata: break
    md5.update(tardata)
  f.close()
  with open(package.script, 'r') as f: md5.update(f.read().encode('utf-8'))
  md5_current = md5.hexdigest()

  # read in the previous MD5
  md5_filename = os.path.join(src_dir, 'md5')
  if os.path.isfile(md5_filename):
    with open(md5_filename, 'r') as f: md5_old = f.read().replace('\n', '')
    # check for any difference
    if md5_old == md5_current and not args.force:
      print('It appears that %s%s %s%s has already been built successfully. '
            'To force a rebuild, use the --force option or delete %s' % (term.bold, package.name,
             package.version, term.end, md5_filename))
      continue

  print('\n%sBuilding %s %s...%s\n' % (term.bold, package.name, package.version, term.end))

  # setup the package install directory
  if not os.path.exists(package_dir): os.makedirs(package_dir)

  # clean the src directory and cd into it
  if os.path.exists(src_dir): shutil.rmtree(src_dir)
  os.makedirs(src_dir)
  os.chdir(src_dir)

  # untar the tar
  ret = subprocess.call('tar -xzf %s' % package.tarball, shell=True)
  if ret != 0: exit(ret)

  # run the build script
  ret = subprocess.call('%s %s' % (package.script, package_dir), shell=True)
  if ret != 0:
    print('')
    error('could not build %s %s. Take a look at %s to debug' % (package.name, package.version, package.script))
    if not package.required:
      print('%snote:%s FreeSurfer can be (limitedly) built without %s. If building it locally is '
            'too troublesome, you can skip it by using the --no-%s flag' % (term.yellow, term.end, package.name, package.name))
    exit(ret)

  # if the build runs smoothly, save the tarball MD5
  with open(md5_filename, "w") as f: f.write(md5_current)

print('\n%sDONE: FreeSurfer packages succesfully built!%s\n' % (term.green, term.end))
