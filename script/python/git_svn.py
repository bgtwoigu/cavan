#!/usr/bin/python

import sys, os, re, time
from getopt import getopt
from xml.dom.minidom import parse, Document

from cavan_file import file_read_line, file_read_lines, \
		file_write_line, file_write_lines, file_append_line, file_append_lines

from cavan_command import CavanCommandBase
from cavan_xml import getFirstElement, getFirstElementData

class SvnInfoParser:
	def loadXml(self, pathname):
		dom = parse(pathname)
		if not dom:
			return False
		self.mRootElement = getFirstElement(dom.documentElement, "entry")
		if self.mRootElement == None:
			return False
		return True

	def getPath(self):
		return self.mRootElement.getAttribute("path")

	def getRevesion(self):
		revision = self.mRootElement.getAttribute("revision")
		if not revision:
			return 0
		return int(revision)

	def getKind(self):
		return self.mRootElement.getAttribute("kind")

	def getUrl(self):
		return getFirstElementData(self.mRootElement, "url")

	def getRepository(self):
		return getFirstElement(self.mRootElement, "repository")

	def getRoot(self):
		tag = self.getRepository()
		if tag == None:
			return None
		return getFirstElementData(tag, "root")

	def getUuid(self):
		tag = self.getRepository()
		if tag == None:
			return None
		return getFirstElementData(tag, "uuid")

	def getWcInfo(self):
		return getFirstElement(self.mRootElement, "wc-info")

	def getWcRootAbsPath(self):
		tag = self.getWcInfo()
		if tag == None:
			return None
		return getFirstElementData(tag, "wcroot-abspath")

	def getSchedule(self):
		tag = self.getWcInfo()
		if tag == None:
			return None
		return getFirstElementData(tag, "schedule")

	def getDepth(self):
		tag = self.getWcInfo()
		if tag == None:
			return None
		return getFirstElementData(tag, "depth")

	def getCommit(self):
		return getFirstElement(self.mRootElement, "commit")

	def getCommitRevision(self):
		tag = self.getCommit()
		if tag == None:
			return None
		return tag.getAttribute("revision")

	def getAuthor(self):
		tag = self.getCommit()
		if tag == None:
			return None
		return getFirstElementData(tag, "author")

	def getDate(self):
		tag = self.getCommit();
		if tag == None:
			return None
		return getFirstElementData(tag, "date")

class SvnLogParser:
	def loadXml(self, pathname):
		dom = parse(pathname)
		if dom == None:
			return False

		self.mRootElement = dom.documentElement
		return True

	def getLogEntrys(self):
		return self.mRootElement.getElementsByTagName("logentry")

class SvnLogEntry:
	def __init__(self, element):
		self.mRootElement = element

	def getRevesion(self):
		return self.mRootElement.getAttribute("revision")

	def getAuthor(self):
		return getFirstElementData(self.mRootElement, "author")

	def getDate(self):
		return getFirstElementData(self.mRootElement, "date")

	def getMessage(self):
		return getFirstElementData(self.mRootElement, "msg")

class GitSvnManager(CavanCommandBase):
	def __init__(self, pathname = "."):
		CavanCommandBase.__init__(self, pathname)
		self.mRemoteName = "cavan-svn"
		self.mPatternSvnUpdate = re.compile('^A[UCGER ]{4}(.*)$')
		self.mPatternGitRevision = re.compile('\s*cavan-git-svn-id: .*@([0-9]+) [^ ]+$')

	def setRootPath(self, pathname):
		CavanCommandBase.setRootPath(self, pathname)
		self.mFileSvnIgnore = self.getAbsPath(".gitignore")

		self.mPathGitSvn = self.getAbsPath(".git/cavan-svn")
		if not os.path.isdir(self.mPathGitSvn):
			os.makedirs(self.mPathGitSvn)

		self.mPathPatch = self.getAbsPath(".git/cavan-patch")
		if not os.path.isdir(self.mPathPatch):
			os.makedirs(self.mPathPatch)

		self.mFileSvnLog = os.path.join(self.mPathGitSvn, "svn_log.xml")
		self.mFileSvnInfo = os.path.join(self.mPathGitSvn, "svn_info.xml")
		self.mFileGitMessag = os.path.join(self.mPathGitSvn, "git_message.txt")

	def genSvnInfoXml(self, url = None):
		if url == None:
			url = self.mUrl

		listCommand = ["svn", "info", "--xml"]

		if url != None:
			listCommand.append(url)

		return self.doExecute(listCommand, of = self.mFileSvnInfo)

	def genSvnLogXml(self):
		if self.mGitRevision >= self.mSvnRevision:
			return False
		return self.doExecute(["svn", "log", "--xml", "-r", "%d:%d" % (self.mGitRevision + 1, self.mSvnRevision), self.mUrl], of = self.mFileSvnLog)

	def setRemoteUrl(self, url):
		return self.doExecute(["git", "config", "remote.%s.url" % self.mRemoteName, url])

	def genGitRepo(self):
		if not CavanCommandBase.genGitRepo(self):
			return False

		return self.setRemoteUrl(self.mUrl)

	def getGitRevision(self):
		lines = self.doSystemPopen("git log -1 | tail -1")
		if not lines:
			return 0

		match = self.mPatternGitRevision.match(lines[0].strip())
		if not match:
			return -1

		return int(match.group(1))

	def saveGitMessage(self, entry):
		content = "%s\n\ncavan-git-svn-id: %s@%s %s" % (entry.getMessage(), self.mUrl, entry.getRevesion(), self.mUuid)
		return file_write_line(self.mFileGitMessag, content)

	def gitCommit(self, entry):
		if self.saveGitMessage(entry) == False:
			return False

		author = entry.getAuthor()
		author = "%s <%s@%s>" % (author, author, self.mUuid)
		if not self.doExecute(["git", "commit", "--author", author, "--date", entry.getDate(), "-aF", self.mFileGitMessag]):
			return False

		self.mGitRevision = int(entry.getRevesion())
		return True

	def listHasPath(self, path, listPath):
		for item in listPath:
			if path.startswith(item):
				return True
		return False

	def gitAddFileList(self, listFile):
		if len(listFile) == 0:
			return True

		listFile.insert(0, "git")
		listFile.insert(1, "add")
		listFile.insert(2, "-f")

		return self.doExecute(listFile, verbose = False)

	def gitAddFiles(self, listUpdate):
		listDir = []
		listFile = []

		for line in listUpdate:
			if self.listHasPath(line, listDir):
				continue

			if os.path.isdir(self.getAbsPath(line)):
				print "[DIR]  Add " + line
				listDir.append(line + "/")
			else:
				print "[FILE] Add " + line
				listFile.append(line)

		if not self.gitAddFileList(listFile):
			return -1

		count = len(listFile)

		for path in listDir:
			lines = self.doPopen(["svn", "list", "-R", path])
			if lines == None:
				return -1

			listFile = []
			for line in lines:
				line = line.rstrip("\r\n")
				if line.endswith("/"):
					continue

				listFile.append(os.path.join(path, line))

			if not self.gitAddFileList(listFile):
				return -1

			count = count + len(listFile)

		return count

	def svnCheckout(self, entry):
		if os.path.isdir(self.getAbsPath(".svn")):
			lines = self.doPopen(["svn", "update", "--accept", "theirs-full", "--force", "-r", entry.getRevesion()])
			if lines == None:
				return False

			listUpdate = []

			for line in lines:
				match = self.mPatternSvnUpdate.match(line)
				if not match:
					continue
				listUpdate.append(match.group(1))

			initialized = True
		else:
			if not self.doExecute(["svn", "checkout", "%s@%s" % (self.mUrl, entry.getRevesion()), "."], of = "/dev/null"):
				return False

			listUpdate = ["."]

			if not os.path.exists(self.mFileSvnIgnore):
				lines = ["/.gitignore\n", ".svn\n"]
				if not file_write_lines(self.mFileSvnIgnore, lines):
					return False

			initialized = False

		count = self.gitAddFiles(listUpdate)
		if count < 0:
			return False

		if self.gitCommit(entry):
			return True

		if count == 0 and initialized == False:
			return True

		lines = self.doPopen(["svn", "diff", "-r", "%d:%s" % (self.mGitRevision, entry.getRevesion())])
		if not lines:
			return lines != None

		return False

	def isInitialized(self):
		return self.doExecute(["git", "branch"], of = "/dev/null")

	def buildSvnUrl(self, url, revision):
		return "%s@%s" % (url, revision)

	def doGitReset(self):
		lines = self.doPopen(["git", "diff"])
		if not lines:
			return True

		tmNow = time.localtime()
		filename = "%04d-%02d%02d-%02d%02d%02d.diff" % (tmNow.tm_year, tmNow.tm_mon, tmNow.tm_mday, tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec)
		file_write_lines(os.path.join(self.mPathPatch, filename), lines)

		return self.doExecute(["git", "reset", "--hard"])

	def doSync(self, url = None):
		self.mGitRevision = self.getGitRevision()
		if self.mGitRevision < 0:
			return False

		if not url:
			lines = self.doPopen(["git", "config", "remote.%s.url" % self.mRemoteName])
			if not lines:
				return False
			url = lines[0].rstrip("\r\n")
		elif not self.setRemoteUrl(url):
			return False

		self.mUrl = url

		if self.genSvnInfoXml() == False:
			return False

		infoParser = SvnInfoParser()
		if infoParser.loadXml(self.mFileSvnInfo) == False:
			return False

		self.mUuid = infoParser.getUuid()
		self.mSvnRevision = infoParser.getRevesion()

		if self.mGitRevision >= self.mSvnRevision:
			self.prGreenInfo("Already up-to-date.")
			return True

		if self.mGitRevision > 0:
			self.doGitReset()
			url = self.buildSvnUrl(self.mUrl, self.mGitRevision)
			if not self.doExecute(["svn", "switch", "--force", "--accept", "theirs-full", url], of = "/dev/null"):
				return False
		else:
			minRevision = 0
			maxRevision = self.mSvnRevision - 1

			while True:
				revision = (minRevision + maxRevision) / 2
				if revision <= minRevision:
					break

				url = self.buildSvnUrl(self.mUrl, revision)
				if self.doExecute(["svn", "info", url], of = "/dev/null", ef = "/dev/null"):
					maxRevision = revision - 1
				else:
					minRevision = revision

			self.mGitRevision = minRevision

		if self.genSvnLogXml() == False:
			return False

		logParser = SvnLogParser()
		if logParser.loadXml(self.mFileSvnLog) == False:
			return False

		nodes = logParser.getLogEntrys();
		if not nodes:
			self.prGreenInfo("Already up-to-date.")
			return True

		for item in logParser.getLogEntrys():
			entry = SvnLogEntry(item)
			if self.svnCheckout(entry) == False:
				return False

		return True

	def doInitBase(self, url, pathname = None):
		self.mUrl = url
		if pathname != None:
			self.setRootPath(pathname)

		if self.isInitialized():
			self.prRedInfo("Has been initialized")
			return False

		if self.mUrl == None:
			if self.genSvnInfoXml() == False:
				return False

			infoParser = SvnInfoParser()
			if infoParser.loadXml(self.mFileSvnInfo) == False:
				return False

			self.mUrl = infoParser.getUrl()

		return self.genGitRepo()

	def doInit(self, argv):
		length = len(argv)
		if length > 0:
			url = argv[0].rstrip("/")
		else:
			url = None

		if length > 1:
			pathname = argv[1].rstrip("/")
		else:
			pathname = None

		return self.doInitBase(url, pathname)

	def doClone(self, argv):
		if len(argv) == 1:
			argv.append(os.path.basename(argv[0].rstrip("/")))

		if not self.doInit(argv):
			return False

		return self.doSync()

	def main(self, argv):
		length = len(argv)
		if length < 2:
			stdio.self.prRedInfo("Please give a subcmd")
			return False

		subcmd = argv[1]
		if subcmd in ["init"]:
			return self.doInit(argv[2:])
		if subcmd in ["clone"]:
			return self.doClone(argv[2:])
		elif subcmd in ["update", "sync"]:
			if length > 2:
				url = argv[2]
			else:
				url = None

			return self.doSync(url)
		else:
			stdio.self.prRedInfo("unknown subcmd " + subcmd)
			return False
