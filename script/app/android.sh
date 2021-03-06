#!/bin/bash

export FILE_APK_RENAME_JAR="${CAVAN_HOME}/java/bin/apkrename.jar"

function cavan-android-get-root()
{
	local android_root

	if [ -d "${ANDROID_BUILD_TOP}" ]
	then
		echo "${ANDROID_BUILD_TOP}"
		return 0
	else
		android_root="${PWD}"

		while [ "${android_root}" != "/" ]
		do
			[ -e "${android_root}/frameworks/base/Android.mk" ] &&
			{
				echo "${android_root}"
				return 0
			}

			android_root=$(dirname "${android_root}")
		done
	fi

	return 1
}

function cavan-android-croot()
{
	local android_root="$(cavan-android-get-root)"

	[ -d "${android_root}" ] && cd "${android_root}"
}

function cavan-android-lunch()
{
	local android_root="$(cavan-android-get-root)"

	[ -d "${android_root}" ] && source "${android_root}/build/envsetup.sh" && lunch $1
}

alias cavan-lunch-rk3288="cavan-android-lunch rk3288-userdebug"
alias cavan-lunch-ms600="cavan-android-lunch imx6ms600-user"

function cavan-sign-update-zip()
{
	local KEY_DIR KEY_NAME FILE_SIGNAPK FILE_INPUT FILE_OUTPUT

	[ "$1" ] ||
	{
		echo "Usage: sign-update-zip update.zip [keyname]"
		return 1
	}

	FILE_INPUT="$1"
	[[ "${FILE_INPUT}" == *.zip ]] ||
	{
		echo "input file '${FILE_INPUT}' is not a .zip file"
		return 1
	}

	[ -f "${FILE_INPUT}" ] ||
	{
		echo "input file '${FILE_INPUT}' is not exists"
		return 1
	}

	FILE_OUTPUT="${FILE_INPUT%.zip}-sign.zip"
	[ -e "${FILE_OUTPUT}" ] &&
	{
		echo "output file '${FILE_OUTPUT}' is exists"
		return 1
	}

	KEY_DIR="build/target/product/security"
	[ -d "${KEY_DIR}" ] ||
	{
		echo "directory '${KEY_DIR}' is not exists"
		return 1
	}

	FILE_SIGNAPK="out/host/linux-x86/framework/signapk.jar"
	[ -f "${FILE_SIGNAPK}" ] ||
	{
		echo "file '${FILE_SIGNAPK}' is not exists"
		return 1
	}

	if [ "$2" ]
	then
		KEY_NAME="$2"
	else
		KEY_NAME="testkey"
	fi

	java -jar "${FILE_SIGNAPK}" -w "${KEY_DIR}/${KEY_NAME}.x509.pem" "${KEY_DIR}/${KEY_NAME}.pk8" "${FILE_INPUT}" "${FILE_OUTPUT}" || return 1

	return 0
}

function cavan-apk-sign()
{
	local KEYSTORE APK_UNSIGNED APK_SIGNED

	[ "$1" ] || return 1

	APK_UNSIGNED="$1"

	if [ "$2" ]
	then
		APK_SIGNED="$2"
	else
		APK_SIGNED="${APK_UNSIGNED}-signed.apk"
	fi

	KEYSTORE="${CAVAN_HOME}/build/core/cavan.keystore"

	jarsigner -digestalg "SHA1" -sigalg "MD5withRSA" -tsa "https://timestamp.geotrust.com/tsa" -storepass "CFA8888" -keystore "${KEYSTORE}" -signedjar "${APK_SIGNED}" "${APK_UNSIGNED}" "${KEYSTORE}"
}

function cavan-apktool()
{
	java -jar "${APKTOOL_JAR}" $@
}

alias apktool="cavan-apktool"

function cavan-apk-decode()
{
	cavan-apktool d -f $@
}

function cavan-apk-encode()
{
	cavan-apktool b -f $@
}

function cavan-apk-pack()
{
	local ROOT_DIR APK_UNSIGNED APK_SIGNED

	if [ "$1" ]
	then
		ROOT_DIR=$(realpath "$1")
	else
		ROOT_DIR="${PWD}"
	fi

	echo "ROOT_DIR = ${ROOT_DIR}"

	APK_UNSIGNED="${ROOT_DIR}/cavan-unsigned.apk"
	APK_SIGNED="${ROOT_DIR}/cavan-signed.apk"

	echo "encode: ${ROOT_DIR} => ${APK_UNSIGNED}"
	rm -f "${APK_UNSIGNED}"
	cavan-apk-encode -o "${APK_UNSIGNED}" "${ROOT_DIR}" || return 1

	echo "signature: ${APK_UNSIGNED} => ${APK_SIGNED}"
	rm -f "${APK_SIGNED}"
	cavan-apk-sign "${APK_UNSIGNED}" "${APK_SIGNED}" || return 1
}

function cavan-apk-rename()
{
	local ROOT_DIR MANIFEST SUFFIX MIME_TYPE IMAGE_PATH SMALI_DIR
	local SOURCE_PKG SOURCE_RE SOURCE_DIR SOURCE_SMALI
	local DEST_PKG DEST_RE DEST_DIR_DIR DEST_SMALI
	local APK_UNSIGNED APK_SIGNED APK_TARGET
	local fn step

	[ -f "${FILE_APK_RENAME_JAR}" ] &&
	{
		java -jar "${FILE_APK_RENAME_JAR}" $@ || return 1
		return 0
	}

	[ "$1" ] ||
	{
		echo "cavan-apk-rename xxxx.apk NAME NAME_NEW"
		return 1
	}

	ROOT_DIR="/tmp/cavan-apk-rename"
	echo "ROOT_DIR = ${ROOT_DIR}"

	rm -rf "${ROOT_DIR}"

	echo "decode: $1 => ${ROOT_DIR}"
	cavan-apk-decode "$1" -o "${ROOT_DIR}" || return 1

	MANIFEST="${ROOT_DIR}/AndroidManifest.xml"
	echo "MANIFEST = ${MANIFEST}"

	if [ "$2" ]
	then
		APK_TARGET="$2"
	else
		APK_TARGET="${ROOT_DIR}/cavan.apk"
	fi

	if [ "$4" ]
	then
		SOURCE_PKG="$3"
		DEST_PKG="$4"
	else
		SOURCE_PKG=$(cat "${MANIFEST}" | grep '\bpackage="[^"]\+"' | sed 's/^.*package="\([^"]\+\)".*$/\1/g')

		if [ "$3" ]
		then
			DEST_PKG="$3"
		else
			DEST_PKG="com.cavan.${SOURCE_PKG}"
		fi
	fi

	echo "rename: ${SOURCE_PKG} => ${DEST_PKG}"

	SOURCE_RE=${SOURCE_PKG//./\\.}
	echo "SOURCE_RE = ${SOURCE_RE}"

	DEST_RE=${DEST_PKG//./\\.}
	echo "DEST_RE = ${DEST_RE}"

	sed -i "s/\(android:name=\"\)\./\1${SOURCE_RE}\./g" "${MANIFEST}" || return 1
	sed -i "s/\(\bpackage=\)\"${SOURCE_RE}\"/\1\"${DEST_PKG}\"/g" "${MANIFEST}" || return 1
	sed -i "s/\(\bandroid:authorities=\"\)${SOURCE_RE}/\1${DEST_PKG}/g" "${MANIFEST}" || return 1

	SMALI_DIR="${ROOT_DIR}/smali"
	echo "SMALI_DIR = ${SMALI_DIR}"

	SOURCE_DIR=${SOURCE_PKG//./\/}
	echo "SOURCE_DIR = ${SOURCE_DIR}"

	SOURCE_SMALI="${SMALI_DIR}/${SOURCE_DIR}"
	echo "SOURCE_SMALI = ${SOURCE_SMALI}"

	DEST_DIR=${DEST_PKG//./\/}
	echo "DEST_DIR = ${DEST_DIR}"

	DEST_SMALI="${SMALI_DIR}_classes2/${DEST_DIR}"
	echo "DEST_SMALI = ${DEST_SMALI}"

	for fn in $(find "${ROOT_DIR}/res" -type f -name "*.xml")
	do
		# echo "Modify file: ${fn}"
		sed -i "s#\b\(xmlns:\w\+=\"http://schemas.android.com/apk/res/\)${SOURCE_RE}#\1${DEST_PKG}#g" "${fn}" || return 1
	done

	for fn in $(find "${ROOT_DIR}/res" -type f)
	do
		MIME_TYPE=$(file -b --mime-type "${fn}")

		case "${MIME_TYPE}" in
			image/png)
				SUFFIX="png";;
			image/jpeg)
				SUFFIX="jpg";;
			image/gif)
				SUFFIX="gif";;
			*)
				continue;;
		esac

		IMAGE_PATH=$(echo "${fn}" | sed "s/\(.*\.\).*$/\1${SUFFIX}/g")
		[ "${fn}" = "${IMAGE_PATH}" ] || mv -v "${fn}" "${IMAGE_PATH}" || return 1
	done

	case "${SOURCE_PKG}" in
		com.qiyi.video)
			for fn in $(find "${SMALI_DIR}" -type f -name "*.smali")
			do
				sed -i "s#\(/data/data/\)${SOURCE_RE}#\1${DEST_PKG}#g" "${fn}" || return 1
				sed -i "s%^\(\s*\)invoke-virtual\s*{\s*[^,]\+,\s*[^,]\+,\s*[^,]\+,\s*\([^}]\+\)},\s*Landroid/content/res/Resources;->getIdentifier(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I%\1const-string/jumbo \2, \"${DEST_PKG}\"\n&%g" "${fn}" || return 1
			done
			;;
		*)
			for fn in $(find "${SMALI_DIR}" -type f -name "*.smali")
			do
				sed -i "s#\(/data/data/\)${SOURCE_RE}#\1${DEST_PKG}#g" "${fn}" || return 1
				sed -i "s#\"${SOURCE_RE}\"#\"${DEST_PKG}\"#g" "${fn}" || return 1
			done
			;;
	esac

	[ -d "${SOURCE_SMALI}" ] &&
	{
		mkdir -p "${DEST_SMALI}" || return 1

		echo "copy: ${SOURCE_SMALI} => ${DEST_SMALI}"
		cp -a "${SOURCE_SMALI}"/* "${DEST_SMALI}" || return 1

		for fn in $(find "${DEST_SMALI}" -type f -name "*.smali")
		do
			sed -i "s#^\(\.class\s\+L\)${SOURCE_DIR}/#\1${DEST_DIR}/#g" "${fn}" || return 1
		done
	}

	APK_UNSIGNED="${ROOT_DIR}/cavan-unsigned.apk"

	echo "encode: ${ROOT_DIR} => ${APK_UNSIGNED}"
	cavan-apk-encode "${ROOT_DIR}" -o "${APK_UNSIGNED}" || return 1

	APK_SIGNED="${ROOT_DIR}/cavan-signed.apk"

	echo "signature: ${APK_UNSIGNED} => ${APK_SIGNED}"
	cavan-apk-sign "${APK_UNSIGNED}" "${APK_SIGNED}" || return 1

	echo "zipalign: ${APK_SIGNED}" "${APK_TARGET}"
	zipalign -v 4 "${APK_SIGNED}" "${APK_TARGET}" || return 1

	echo "File stored in: ${APK_TARGET}"
}

function cavan-apk-rename-auto()
{
	local ROOT_DIR APK_DEST APK_FAILED FAILED_DIR BASE_NAME

	[ -f "${FILE_APK_RENAME_JAR}" ] &&
	{
		java -jar "${FILE_APK_RENAME_JAR}" $@ || return 1
		return 0
	}

	ROOT_DIR="${!#}"
	echo "ROOT_DIR = ${ROOT_DIR}"

	FAILED_DIR="${ROOT_DIR}/failure"
	echo "FAILED_DIR = ${FAILED_DIR}"

	mkdir -p "${ROOT_DIR}" || return 1
	mkdir -p "${FAILED_DIR}" || return 1

	while [ "$2" ]
	do
		echo "================================================================================"

		BASE_NAME=$(basename -s .apk "$1")
		APK_DEST="${ROOT_DIR}/${BASE_NAME}-cavan.apk"
		APK_FAILED="${FAILED_DIR}/${BASE_NAME}.apk"

		echo "rename: $1 => ${APK_DEST}"

		if [ -f "${APK_DEST}" ]
		then
			echo "skip exist file: ${APK_DEST}"
		elif cavan-apk-rename "$1" "${APK_DEST}"
		then
			rm "${APK_FAILED}"
		else
			cp -av "$1" "${APK_FAILED}" || return 1
			sleep 1
		fi

		shift
	done
}

function cavan-adb-logcat()
{
	cavan-loop_run -wd2 "adb logcat -v threadtime $@" || return 1
}

function cavan-adb-logcat-error()
{
	cavan-adb-logcat -s "System,System.err,AndroidRuntime" "*:e"
}

function cavan-adb-loop_run()
{
	cavan-loop_run -wd2 "adb root && adb wait-for-device && adb remount; adb shell $*" || return 1
}

function cavan-adb-cavan-main()
{
	adb push ${CMD_ARM_CAVAN_MAIN} ${CMD_DATA_CAVAN_MAIN} || return 1
	adb shell "chmod 777 ${CMD_DATA_CAVAN_MAIN} && ${CMD_DATA_CAVAN_MAIN} \"$*\"" || return 1
}

function cavan-adb-tcp_dd_server()
{
	cavan-loop_run -wd2 "adb remount; adb shell cavan-main tcp_dd_server"
}

function cavan-adb-kmsg()
{
	cavan-adb-loop_run "cat /proc/kmsg" || return 1
}

function cavan-adb-build-env()
{
	adb remount || return 1
	adb push ${CMD_ARM_CAVAN_MAIN} ${CMD_SYSTEM_CAVAN_MAIN} || return 1
	adb shell chmod 06777 ${CMD_SYSTEM_CAVAN_MAIN} || return 1

	adb shell chmod 777 /data/bin/bash || return 1
	adb shell cp /data/bin/bash /system/bin/sh || return 1
}

function cavan-make-apk()
{
	[ -f Makefile ] ||
	{
		ln -sf ${CAVAN_HOME}/build/core/apk_main.mk Makefile
	}

	make
}

function cavan-mm-apk()
{
	local pkg_name apk_name

	[ -f "AndroidManifest.xml" ] || return 1

	pkg_name=$(cat AndroidManifest.xml | grep "package=" | sed 's/.*package="\([^"]\+\)"/\1/g')
	apk_name=$(basename ${PWD})

	echo "pkg_name = ${pkg_name}"

	[ -e "Android.mk" ] || cat > Android.mk << EOF
LOCAL_PATH := \$(call my-dir)

include \$(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \$(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := ${apk_name}
LOCAL_CERTIFICATE := platform

include \$(BUILD_PACKAGE)
EOF

	[ -e "Makefile" ] || cat > Makefile << EOF
install: uninstall
	adb install \$(ANDROID_PRODUCT_OUT)/system/app/${apk_name}.apk

uninstall:
	adb uninstall ${pkg_name}

.PHONE: uninstall
EOF
}

function cavan-adb-push-directory()
{
	local fn

	cd $1 || return 1

	for fn in *
	do
		[ -f "${fn}" ] || continue
		echo "Push ${fn} => $2"
		adb push ${fn} $2 || return 1
	done

	return 0
}

function cavan-android-keystore-create()
{
	local keystore=${1-"debug.keystore"}

	keytool -v -genkey -dname "CN=Fuang Cao, OU=Cavan, O=Cavan, L=Shanghai, ST=Shanghai, C=CN" -alias androiddebugkey -storepass android -keypass android -keyalg RSA -validity 80000 -keystore "${keystore}"
}

function cavan-android-keystore-show()
{
	[ "$1" ] || return 1

	keytool -v -list -storepass android -keystore "$1"
}
