/*
 * Author: Fuang.Cao
 * Email: cavan.cfa@gmail.com
 * Date: Thu Sep  6 10:14:09 CST 2012
 */

#include <cavan.h>
#include <alsa/asoundlib.h>
#include <cavan/sound.h>

#define FILE_CREATE_DATE "2012-09-06 10:14:09"

enum
{
	LOCAL_COMMAND_OPTION_UNKNOWN,
	LOCAL_COMMAND_OPTION_HELP,
	LOCAL_COMMAND_OPTION_VERSION,
};

int main(int argc, char *argv[])
{
	return cavan_wav_playback(argv[1]);
}
