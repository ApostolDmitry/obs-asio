/* openASIO SDK v2
 * compatibility layer to allow compilation of ASIO hosts in GPL compliant way
 * asio-wrapper.hpp
 *
 * current version vs v1: stripped down version sufficient to compile the obs plugin
 *
 * Copyright (c) 2023   pkv <pkv.stream@gmail.com> Andersama <anderson.john.alexander@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 ******************************************************************************
 * Disclaimer:                                                                *
 ******************************************************************************
 * None of the contents of this file were derived from Steinberg ASIO sdk.    *
 * The content was derived from analysis and cross-examination of asio hosts, *
 * drivers, wrapper API, forum discussions, etc                               *
 * For headers the sources for each item are given in comments.               *
 ******************************************************************************
 * Most of the general info on asio sdk structure was drawn from this paper:
 *
 * https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/Pa_ASIO.pdf
 *
 * The starting point for this implementation is Synchronous Audio Router tinyASIO (released under GPL v3):
 * https://github.com/eiz/SynchronousAudioRouter/blob/master/SarAsio/tinyasio.h
 * (function InstalledAsioDrivers() which queries the windows Registry)
 *
 * Links to wrapper APIs, asio hosts or asio drivers where we found infos:
 *
 * COM interface:
 * https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/iasiothiscallresolver.cpp
 *
 * c/c++ wrappers and audio API:
 * https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp
 * https://www.codeguru.com/cpp/cpp/cpp_mfc/oop/article.php/c15891/A-Multidevice-ASIO-Output-Plugin-for-WinAMP.htm and Wasiona_source.zip
 * https://github.com/WeAreROLI/JUCE/blob/master/modules/juce_audio_devices/native/juce_win32_ASIO.cpp
 * https://github.com/sjoerdvankreel/xt-audio/blob/master/src/core/xt-asio.cpp
 *
 * java wrapper:
 * http://jsasio.sourceforge.net/
 *
 * .NET wrapper:
 * http://www.bass.radio42.com/help/html/7740a793-3341-3e90-c62c-c7bee8311772.htm
 *
 * c# wrapper:
 * https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs
 *
 * delphi wrapper:
 * http://lakeofsoft.com/vc/doc/unaASIOAPI.html
 *
 * Golang wrapper:
 * https://github.com/JamesDunne/go-asio/blob/master/asio.go
 *
 * c/c++ driver:
 * https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp
 * https://github.com/eiz/SynchronousAudioRouter/blob/master/SarAsio/tinyasio.h
 *
 * c/c++ hosts:
 * https://github.com/steveschow/osxwineasio/blob/master/asio.c
 * https://github.com/t-mat/vsthost-vc2013/blob/master/vsthostsrc/AsioHost.cpp
 * http://kana-soft.com/tech/sample_0004_5.htm
 * https://github.com/dechamps/FlexASIO/blob/master/flexasio.h
 *
 * Additional links given below.
 *
 ******************************************************************************
 * Legal Disclaimer:                                                          *
 * use this file in your project only if your jurisdiction allows it          *
 ******************************************************************************
 *	USA: complies with Section 103(f) of the Digital Millennium Copyright Act
 *	(DMCA) (17 USC � 1201 (f) - Reverse Engineering) which allows reverse
 *	engineering for the purpose of interoperability between computer programs
 *	(such as information transfer between applications). Interoperability is
 *	defined in paragraph 4 of Section 103(f).
 *		Paragraph (1) of Section 103(f):
 *		(1) Notwithstanding the provisions of subsection (a)(1)(A), a person who
 *		has lawfully obtained the right to use a copy of a computer program may
 *		circumvent a technological measure that effectively controls access to a
 *		particular portion of that program for the sole purpose of identifying
 *		and analyzing those elements of the program that are necessary to
 *		achieve interoperability of an independently created computer program with
 *		other programs, and that have not previously been readily available to the
 *		 person engaging in the circumvention, to the extent any such acts of
 *		identification and analysis do not constitute infringement under this title.
 *
 *	EU: complies with EU Directive 2009/24 which superseded Article 6 of the 1991
 *	EU Computer Programs Directive.
 *	Article (15) allows reverse engineering for the purposes of interoperability :
 *		circumstances may exist when such a reproduction of the code and
 *		translation of its form are indispensable to obtain the necessary
 *		information to achieve the interoperability of an independently created
 *		program with other programs.
 *		It has therefore to be considered that, in these limited circumstances
 *		only, performance of the acts of reproduction and translation by or on
 *		behalf of a person having a right to use a copy of the program is
 *		legitimate and compatible with fair practice and must therefore be deemed
 *		not to require the authorisation of the rightholder.
 *		An objective of this exception is to make it possible to connect all
 *		components of a computer system, including those of different
 *		manufacturers, so that they can work together.
 */
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include "windows.h"
#include <locale>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <codecvt>
#include <cstddef>
#include <cstdint>

// define added for iasiothiscallresolver.h
#ifndef __ASIO_H
#define __ASIO_H

/******************************************************************************/
/*              list drivers from registry, initialize COM                    */
/*            we use tinyasio implementation (GPL v2 or later)                */
/*     SynchronousAudioRouter/blob/master/SarAsio/tinyasio.cpp#ln23           */
/******************************************************************************/

/* http://www.oifii.org/ns-org/nsd/ar/cp/audio_spivsthost/vsthostsrc/AsioHost.cpp */
/* https://github.com/t-mat/vsthost-vc2013/blob/master/vsthostsrc/AsioHost.cpp line 665 */
#define ASIO_LITTLE_ENDIAN true
/* the following from https://www.freebasic.net/forum/viewtopic.php?t=16890 */
#define NATIVE_INT64				0
#define IEEE754_64FLOAT				1
#define DRVERR						-5000
#define DRVERR_INVALID_PARAM		DRVERR-1
#define DRVERR_DEVICE_ALREADY_OPEN	DRVERR-2
#define DRVERR_DEVICE_NOT_FOUND		DRVERR-3
#define MAXPATHLEN					512
#define MAXDRVNAMELEN				128

/*
 * https://github.com/centrevillage/jsasio/blob/master/jsASIO1.2/com/groovemanager/spi/asio/ASIOError.java
 * values of enum found in http://lakeofsoft.com/vc/doc/unaASIOAPI.html
 * https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln13
 */
typedef long ASIOError;
enum {
	ASE_OK = 0,
	ASE_NotPresent = -1000, // http://lakeofsoft.com/vc/doc/unaASIOAPI.html
	ASE_HWMalfunction, // ASE_NotPresent + 1 in http://lakeofsoft.com/vc/doc/unaASIOAPI.html
	ASE_InvalidParameter, // ASE_NotPresent + 2 in http://lakeofsoft.com/vc/doc/unaASIOAPI.html
	ASE_InvalidMode, // ASE_NotPresent + 3 in http://lakeofsoft.com/vc/doc/unaASIOAPI.html
	ASE_SPNotAdvancing, // ASE_NotPresent + 4 in http://lakeofsoft.com/vc/doc/unaASIOAPI.html
	ASE_NoClock, // ASE_NotPresent + 5 in http://lakeofsoft.com/vc/doc/unaASIOAPI.html
	ASE_NoMemory, // ASE_NotPresent + 6 in http://lakeofsoft.com/vc/doc/unaASIOAPI.html
	ASE_SUCCESS = 0x3f4847a0, // http://lakeofsoft.com/vc/doc/unaASIOAPI.html
};

struct ASIOInternalBufferInfo
{
	long inputSamples; // size of driver's internal input buffering which is included in ASIOGetLatencies
	long outputSamples; // size of driver's internal output bufferingwhich is included in ASIOGetLatencies
};

/* https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/iasiothiscallresolver.cpp
 */
 typedef interface IASIO IASIO;

/* see https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2913
 * ASIOBool can take value ASIOTrue
 * from http://lakeofsoft.com/vc/doc/unaASIOAPI.html#ASIOBool ==> type long
 * also https://github.com/eiz/SynchronousAudioRouter/blob/master/SarAsio/tinyasio.h#ln22
 */
typedef long ASIOBool;
enum {
	ASIOFalse = false,
	ASIOTrue = true
};

/* from https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln1955
 * type is float or double
 * is a double from https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#184
 */
typedef double ASIOSampleRate;

/* from https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln345
 * and ln405
 * enum order also: https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln72
 * lot of details also here
 * https://github.com/WeAreROLI/JUCE/blob/master/modules/juce_audio_devices/native/juce_win32_ASIO.cpp#ln80
 * LSB types are little endian, MSB are big endian quite likely, we haven't found them used in the
 * hosts/drivers we studied.
 */
typedef long ASIOSampleType;
enum {
	ASIOSTInt16MSB = 0,
	ASIOSTInt24MSB = 1,
	ASIOSTInt32MSB = 2,
	ASIOSTFloat32MSB = 3,
	ASIOSTFloat64MSB = 4,
	ASIOSTInt32MSB16 = 8,
	ASIOSTInt32MSB18 = 9,
	ASIOSTInt32MSB20 = 10,
	ASIOSTInt32MSB24 = 11,
	ASIOSTInt16LSB = 16,
	ASIOSTInt24LSB = 17,
	ASIOSTInt32LSB = 18,
	ASIOSTFloat32LSB = 19,
	ASIOSTFloat64LSB = 20,
	ASIOSTInt32LSB16 = 24,
	ASIOSTInt32LSB18 = 25,
	ASIOSTInt32LSB20 = 26,
	ASIOSTInt32LSB24 = 27,
	ASIOSTDSDInt8LSB1 = 32,
	ASIOSTDSDInt8MSB1 = 33,
	ASIOSTDSDInt8NER8 = 40,
	ASIOSTLastEntry = 41, // from http://lakeofsoft.com/vc/doc/unaASIOAPI.html#ASIOSTLastEntry
};

/* from https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln125
 * also https://github.com/eiz/SynchronousAudioRouter/blob/master/SarAsio/tinyasio.h#ln37
 * see https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln3202
 */
enum ASIOMessageSelector;
enum
{
	kAsioSelectorSupported = 1,
	kAsioEngineVersion,
	kAsioResetRequest,
	kAsioBufferSizeChange,
	kAsioResyncRequest,
	kAsioLatenciesChanged,
	kAsioSupportsTimeInfo,
	kAsioSupportsTimeCode,
	kAsioMMCCommand,
	kAsioSupportsInputMonitor,
	kAsioSupportsInputGain,
	kAsioSupportsInputMeter,
	kAsioSupportsOutputGain,
	kAsioSupportsOutputMeter,
	kAsioOverload,
};

/* http://jsasio.sourceforge.net/com/groovemanager/spi/asio/ASIOClockSource.html
 * http://lakeofsoft.com/vc/doc/unaASIOAPI.ASIOClockSource.html
 * https://github.com/eiz/SynchronousAudioRouter/blob/master/SarAsio/tinyasio.h#ln63
 */
typedef struct ASIOClockSource
{
	long index; //https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln487
	long associatedChannel; //https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln488
	long associatedGroup; /* https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln489
	type: https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln1924 */
	ASIOBool isCurrentSource; /* https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln490
	 type is ASIOBool because can take value ASIOTrue */
	char name[32]; /* https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln491
	and array size: http://lakeofsoft.com/vc/doc/unaASIOAPI.ASIOClockSource.html */
} ASIOClockSource;

/* from https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2896
and also line 57 https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln57
type inferred from https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln152 , ln153, ln160 */
typedef struct ASIOSamples {
	unsigned long hi;
	unsigned long lo;
} ASIOSamples;

/* same as previous struct
 * https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2896
 */
typedef struct ASIOTimeStamp {

	unsigned long hi;
	unsigned long lo;
} ASIOTimeStamp;

/* elements names and types from:
 * https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln98
 * https://github.com/eiz/SynchronousAudioRouter/blob/master/SarAsio/tinyasio.h#ln72
 * additional cross-refs given
 */
typedef struct ASIOChannelInfo
{
	long channel; /* also https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln1123
	and from https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln563 */
	ASIOBool isInput; /* also https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln1124 (ASIOTrue ==> type ASIOBool = bool)*/
	ASIOBool isActive; /* https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln576 ==> same type as previous, so ASIOBool */
	long channelGroup; //https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln566
	ASIOSampleType type; /* https://github.com/nikkov/Win-Widget/blob/master/Driver/asiouac2.cpp#ln553
	 and from https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2412 we know the type is ASIOSampleType
	 as well as from ln 405 BytesPerAsioSample function called at line 1655 */
	char name[32]; /* https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln4161 ;
	array size from https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln107 */
} ASIOChannelInfo;

/* https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln111
 * https://github.com/eiz/SynchronousAudioRouter/blob/master/SarAsio/tinyasio.h#ln82
 */
typedef struct ASIOBufferInfo
{
	ASIOBool isInput; /*also https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2235 , type ASIOBool since it can be ASIOTrue */
	long channelNum; /* also from: https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2240*/
	void *buffers[2]; /* also from https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln1653
	 ln 3299 , ln 3300 confirm length of array */
} ASIOBufferInfo;

/* https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln168
 * also http://jsasio.sourceforge.net/com/groovemanager/spi/asio/ASIOTimeCode.html
 */
enum ASIOTimeCodeFlags;
enum {

	kTcValid = 1,
	kTcRunning = 1 << 1,
	kTcReverse = 1 << 2,
	kTcOnspeed = 1 << 3,
	kTcStill = 1 << 4,
	kTcSpeedValid = 1 << 8
};


/* from https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln149
 * https://github.com/eiz/SynchronousAudioRouter/blob/master/SarAsio/tinyasio.h#ln89
 */
typedef struct ASIOTimeCode
{
	double          speed;//also from https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2943
	ASIOSamples     timeCodeSamples; /* type is from http://jsasio.sourceforge.net/com/groovemanager/spi/asio/ASIOTimeCode.html#timeCodeSamples()
	type found also in http://lakeofsoft.com/vc/doc/unaASIOAPI.ASIOTimeCode.html
	From https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2943
	This confirms ASIOSamples = ASIO64bit from portaudio and is 64bit.
	*/
	ASIOTimeCodeFlags   flags; //https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln154
	char future[64]; //https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln155 for array size & ln156
} ASIOTimeCode;

/* https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln191
 */
typedef long AsioTimeInfoFlags;
enum
{
	kSystemTimeValid = 1,
	kSamplePositionValid = 1 << 1,
	SampleRateValid = 1 << 2,
	kSpeedValid = 1 << 3,
	kSampleRateChanged = 1 << 4,
	kClockSourceChanged = 1 << 5
};

/*
 * https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln179 and following
 * http://jsasio.sourceforge.net/com/groovemanager/spi/asio/AsioTimeInfo.html
 * https://github.com/eiz/SynchronousAudioRouter/blob/master/SarAsio/tinyasio.h#ln97
 */
typedef struct AsioTimeInfo
{
	double				speed; //https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln181
	ASIOTimeStamp		systemTime; //also  https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2933
	ASIOSamples			samplePosition; //also https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/pa_asio.cpp#ln2938
	ASIOSampleRate		sampleRate; // is a double from https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln184
	AsioTimeInfoFlags	flags; // https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln185
	char reserved[12]; // https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln186
} AsioTimeInfo;

// from https://github.com/SjB/NAudio/blob/master/NAudio/Wave/Asio/ASIOStructures.cs#ln202
typedef struct ASIOTime
{
	long reserved[4];
	struct AsioTimeInfo	timeInfo;
	struct ASIOTimeCode	timeCode;
} ASIOTime;

/* params for asiofuture
 * from http://www.bass.radio42.com/help/html/1b592b67-9f60-4f5b-1497-7e32666485de.htm
 * (for enum and description)
 * list from https://github.com/steveschow/osxwineasio/blob/master/asio.c#ln1205
 * also http://jsasio.sourceforge.net/com/groovemanager/spi/asio/ASIOStaticFunctions.html
 * see also http://kana-soft.com/tech/sample_0004_5.htm  where the explanations were extracted
 * (see also // from http://lakeofsoft.com/vc/doc/unaASIOAPI.html which confirms enum starts at 1)
 */
enum
{
	kAsioEnableTimeCodeRead = 1,		// There is no parameter .
	kAsioDisableTimeCodeRead,			// There is no parameter .
	kAsioSetInputMonitor,				// Parameter ASIOInputMonitor structure pointer ; Return value OK = ASE_SUCCESS / Error = ASE_NotPresent
	kAsioTransport,						// Parameter is a ASIOTransportParameters*
	kAsioSetInputGain,					// Parameter is a ASIOChannelControls*
	kAsioGetInputMeter,					// Parameter is a ASIOChannelControls*
	kAsioSetOutputGain,					// Parameter is a ASIOChannelControls*
	kAsioGetOutputMeter,				// Parameter is a ASIOChannelControls*
	kAsioCanInputMonitor,				// There is no parameter .
	kAsioCanTimeInfo,					// There is no parameter .
	kAsioCanTimeCode,					// There is no parameter .
	kAsioCanTransport,					// There is no parameter .
	kAsioCanInputGain,					// There is no parameter .
	kAsioCanInputMeter,					// There is no parameter .
	kAsioCanOutputGain,					// There is no parameter .
	kAsioCanOutputMeter,				// There is no parameter .
/*
	kAsioOptionalOne = ??? from https://github.com/Mizunagi/AudioSystem/blob/e7e449b273872715fd2dcabf86ab7ca9fa458a9b/html/asio_8h.html#ln294
*/
	// from http://kana-soft.com/tech/sample_0004_5.htm and http://lakeofsoft.com/vc/doc/unaASIOAPI.html
	kAsioSetIoFormat = 0x23111961,		/* Change the I / O format of the device.
 - A pointer to the parameter ;
   format type ( ASIOIoFormat structure ).
 - Return value : OK = ASE_SUCCESS / Error = ASE_NotPresent           */
	kAsioGetIoFormat = 0x23111983,		/* Get the I / O format of the device.
 - A pointer to the parameter; format type ( ASIOIoFormat structure ).
 - Return value : OK = ASE_SUCCESS / Error = ASE_NotPresent */
	kAsioCanDoIoFormat = 0x23112004,	/* Query whether the I / O format of the specified device is available or not.
 - A pointer to the parameter; format type ( ASIOIoFormat structure ).
 - Return value : OK = ASE_SUCCESS / Error = ASE_NotPresent */
/*	kAsioCanReportOverload = ??? from https://forum.juce.com/t/solved-kasiocanreportoverload-undeclared-identifier/25019/3
 and https://github.com/WeAreROLI/JUCE/blob/master/modules/juce_audio_devices/native/juce_win32_ASIO.cpp#ln469
	kAsioGetInternalBufferSamples = ??? from https://github.com/Mizunagi/AudioSystem/blob/e7e449b273872715fd2dcabf86ab7ca9fa458a9b/html/asio_8h.html#ln300
*/
	kAsioCanReportOverload = 0x24042012,
	kAsioGetInternalBufferSamples = 0x25042012,
};

/* https://www.codeguru.com/cpp/cpp/cpp_mfc/oop/article.php/c15891/A-Multidevice-ASIO-Output-Plugin-for-WinAMP.htm
 * and Wasiona_source.zip : WASIO\ASIODevice.hpp(ln 202 and 208)
*/
typedef struct ASIOCallbacks
{
	void(*bufferSwitch) (long doubleBufferIndex, ASIOBool directProcess);
	void(*sampleRateDidChange) (ASIOSampleRate sRate);
	long(*asioMessage) (long selector, long value, void* message, double* opt);
	ASIOTime* (*bufferSwitchTimeInfo) (ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
} ASIOCallbacks;

// from: https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/iasiothiscallresolver.cpp
interface IASIO : public IUnknown
{

	virtual ASIOBool init(void *sysHandle) = 0;
	virtual void getDriverName(char *name) = 0;
	virtual long getDriverVersion() = 0;
	virtual void getErrorMessage(char *string) = 0;
	virtual ASIOError start() = 0;
	virtual ASIOError stop() = 0;
	virtual ASIOError getChannels(long *numInputChannels, long *numOutputChannels) = 0;
	virtual ASIOError getLatencies(long *inputLatency, long *outputLatency) = 0;
	virtual ASIOError getBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity) = 0;
	virtual ASIOError canSampleRate(ASIOSampleRate sampleRate) = 0;
	virtual ASIOError getSampleRate(ASIOSampleRate *sampleRate) = 0;
	virtual ASIOError setSampleRate(ASIOSampleRate sampleRate) = 0;
	virtual ASIOError getClockSources(ASIOClockSource *clocks, long *numSources) = 0;
	virtual ASIOError setClockSource(long reference) = 0;
	virtual ASIOError getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp) = 0;
	virtual ASIOError getChannelInfo(ASIOChannelInfo *info) = 0;
	virtual ASIOError createBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks *callbacks) = 0;
	virtual ASIOError disposeBuffers() = 0;
	virtual ASIOError controlPanel() = 0;
	virtual ASIOError future(long selector,void *opt) = 0;
	virtual ASIOError outputReady() = 0;
};

#endif
