package com.cavan;

import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

import com.cavan.java.CavanFile;
import com.cavan.java.CavanJava;

public class ApkRenameMain {

	public static final Path DEFAULT_WORK_PATH = Paths.get("/tmp", "cavan-apk-rename-auto");

	private CavanFile mDirOut;
	private CavanApkMapFile mFileFailure;
	private CavanApkMapFile mFileSuccessfull;
	private List<String> mSourceApks = new ArrayList<String>();

	public ApkRenameMain(CavanFile inFile, CavanFile outFile) {
		if (inFile.isDirectory()) {
			mDirOut = new CavanFile(outFile, inFile.getName());
		} else {
			mDirOut = outFile;
		}

		CavanJava.logD("mDirOut = " + mDirOut.getPath());

		mFileFailure = new CavanApkMapFile(mDirOut, "failure.txt");
		mFileFailure.load();

		mFileSuccessfull = new CavanApkMapFile(mDirOut, "successfull.txt");
		mFileSuccessfull.load();

		addApk(inFile);
	}

	public ApkRenameMain(String inPath, CavanFile outFile) {
		this(new CavanFile(inPath), outFile);
	}

	public void addApk(File file) {
		if (file.isDirectory()) {
			addApkDir(file);
		} else {
			mSourceApks.add(file.getPath());
		}
	}

	private void addApkDir(File dir) {
		for (File file : dir.listFiles()) {
			addApk(file);
		}
	}

	public String buildApkName(String filename, String appName) {
		for (char c : new char[] { '-', '_', ' ' }) {
			int index = filename.indexOf(c);
			if (index > 0) {
				return appName + filename.substring(index);
			}
		}

		return appName + ".apk";
	}

	public CavanFile buildApkFile(String filename, String appName) {
		String apkName = buildApkName(filename, appName);
		CavanFile apkFile = new CavanFile(mDirOut, apkName);
		if (apkFile.exists()) {
			if (filename.equals(apkName)) {
				return null;
			}

			return new CavanFile(mDirOut, appName + "-" + filename);
		}

		return apkFile;
	}

	public boolean doRenameFile(File inFile) {
		String filename = inFile.getName();
		if (mFileSuccessfull.hasApk(filename)) {
			CavanJava.logD("skip exists file: " + inFile.getPath());
			return true;
		}

		if (mFileFailure.hasApk(filename)) {
			CavanJava.logD("skip error file: " + inFile.getPath());
			return true;
		}

		CavanFile outFile = new CavanFile(mDirOut, filename);
		ApkRename rename = new ApkRename(new File(DEFAULT_WORK_PATH.toString()), inFile, outFile);
		boolean success = rename.doRename();
		String appName = rename.getAppName();
		if (success) {
			if (appName != null) {
				CavanFile namedFile = buildApkFile(filename, appName);
				if (namedFile != null) {
					CavanJava.logD("move: " + outFile.getPath() + " => " + namedFile.getPath());
					if (!outFile.renameTo(namedFile)) {
						CavanJava.logP("Failed to renameTo: " + namedFile.getPath());
						return false;
					}

					outFile = namedFile;
				}
			}

			mFileSuccessfull.addApk(filename, appName);
		} else {
			CavanJava.logP("Failed to doRename: " + inFile.getPath());
			mFileFailure.addApk(filename, appName);
		}

		return true;
	}

	public boolean doRenameDir(File inDir) {
		for (File file : inDir.listFiles()) {
			if (file.isDirectory()) {
				if (!doRenameDir(file)) {
					return false;
				}
			} else if (!doRenameFile(file)) {
				return false;
			}
		}

		return true;
	}

	public boolean doRename() {
		if (!mDirOut.mkdirsSafe()) {
			CavanJava.logP("Failed to mkdirsSafe: " + mDirOut.getPath());
			return false;
		}

		int count = 1;
		int error = 0;

		for (String pathname : mSourceApks) {
			CavanJava.logD("rename file: " + pathname + " (" + count + "/" + mSourceApks.size() + ")");

			File file = new File(pathname);
			if (!doRenameFile(file)) {
				CavanJava.logE("Failed to rename file: " + file.getAbsolutePath());
				error++;
			}

			CavanJava.printSep();
			count++;
		}

		if (error > 0) {
			CavanJava.logE("rename failure count = " + error);
			return false;
		}

		CavanJava.logD("rename successfull");

		return true;
	}

	public static void main(String[] args) throws Exception {
		boolean success = false;

		if (args.length > 1) {
			int	count = args.length - 1;
			CavanFile outDir = new CavanFile(args[count]);

			for (int i = 0; i < count; i++) {
				ApkRenameMain rename = new ApkRenameMain(args[i], outDir);
				success = rename.doRename();
				if (!success) {
					break;
				}
			}
		} else if (args.length > 0) {
			ApkRename rename = new ApkRename(args[0]);
			success = rename.doRename();
		} else {
			CavanJava.logD("apkrename <IN_APK> ... [OUT_APK]");
		}

		if (!success) {
			throw new Exception("Failed to ApkRename");
		}
	}
}
