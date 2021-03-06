import os
import os.path
import tempfile

import librepo
import _librepo_test

#EXPECT_SYSTEM_NSOLVABLES = _hawkey_test.EXPECT_SYSTEM_NSOLVABLES
#EXPECT_MAIN_NSOLVABLES = _hawkey_test.EXPECT_MAIN_NSOLVABLES
#EXPECT_UPDATES_NSOLVABLES = _hawkey_test.EXPECT_UPDATES_NSOLVABLES
#EXPECT_YUM_NSOLVABLES = _hawkey_test.EXPECT_YUM_NSOLVABLES
#FIXED_ARCH = _hawkey_test.FIXED_ARCH
#UNITTEST_DIR = _hawkey_test.UNITTEST_DIR
#YUM_DIR_SUFFIX = _hawkey_test.YUM_DIR_SUFFIX

#glob_for_repofiles = _hawkey_test.glob_for_repofiles

#cachedir = None
# inititialize the dir once and share it for all sacks within a single run
#if cachedir is None:
#    cachedir = tempfile.mkdtemp(dir=os.path.dirname(UNITTEST_DIR),
#                                prefix='pyhawkey')

#class TestSackMixin(object):
#    def __init__(self, testdata_dir):
#        self.testdata_dir = testdata_dir
#
#    def load_test_repo(self, name, fn):
#        path = os.path.join(self.testdata_dir, fn)
#        _hawkey_test.load_repo(self, name, path, False)
#
#    def load_system_repo(self, *args, **kwargs):
#        path = os.path.join(self.testdata_dir, "@System.repo")
#        _hawkey_test.load_repo(self, hawkey.SYSTEM_REPO_NAME, path, True)
#
#    def load_yum_repo(self, **args):
#        d = os.path.join(self.testdata_dir, YUM_DIR_SUFFIX)
#        repo = glob_for_repofiles(self, "messerk", d)
#        super(TestSackMixin, self).load_yum_repo(repo, **args)
#
#class TestSack(TestSackMixin, hawkey.Sack):
#    def __init__(self, testdata_dir, PackageClass=None, package_userdata=None):
#        TestSackMixin.__init__(self, testdata_dir)
#        hawkey.Sack.__init__(self,
#                             cachedir=cachedir,
#                             arch=FIXED_ARCH,
#                             pkgcls=PackageClass,
#                             pkginitval=package_userdata)
