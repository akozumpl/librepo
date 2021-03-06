REPOS_YUM = "yum/static/"
HARMCHECKSUM = "yum/harm_checksum/%s/"
MISSINGFILE = "yum/not_found/%s/"
BADURL = "yum/badurl/"
BADGPG = "yum/badgpg/"
AUTHBASIC = "yum/auth_basic/"
PARTIAL = "yum/partial/"

AUTH_USER = "admin"
AUTH_PASS = "secret"

REPO_YUM_01_PATH = REPOS_YUM+"01/"
REPO_YUM_01_REPODATA = REPO_YUM_01_PATH+"repodata/"
REPO_YUM_01_REPOMD = REPO_YUM_01_REPODATA+"repomd.xml"

REPO_YUM_02_PATH = REPOS_YUM+"02/"

METALINK_DIR = "yum/static/metalink/"
METALINK_GOOD_01 = METALINK_DIR+"good_01.xml"
METALINK_GOOD_02 = METALINK_DIR+"good_02.xml"
METALINK_BADFILENAME = METALINK_DIR+"badfilename.xml"
METALINK_BADCHECKSUM = METALINK_DIR+"badchecksum.xml"
METALINK_NOURLS = METALINK_DIR+"nourls.xml"
METALINK_BADFIRSTURL = METALINK_DIR+"badfirsturl.xml"
METALINK_FIRSTURLHASCORRUPTEDFILES = METALINK_DIR+"firsturlhascorruptedfiles.xml"

MIRRORLIST_DIR = "yum/static/mirrorlist/"
MIRRORLIST_GOOD_01 = MIRRORLIST_DIR+"good_01"
MIRRORLIST_GOOD_02 = MIRRORLIST_DIR+"good_02"
MIRRORLIST_NOURLS = MIRRORLIST_DIR+"nourls"
MIRRORLIST_BADFIRSTURL = MIRRORLIST_DIR+"badfirsturl"
MIRRORLIST_FIRSTURLHASCORRUPTEDFILES = MIRRORLIST_DIR+"firsturlhascorruptedfiles"

# PACKAGE_<repo_id>_<package_id>
REPO_YUM_01_PACKAGES = REPO_YUM_01_PATH
PACKAGE_01_01 = "filesystem-2.4.44-1.fc16.i686.rpm"
PACKAGE_01_01_SHA256 = "3a2d4370e617056c9f76ec9ad1a7c699db8da90d3cc860943035ca0fda8136f4"
