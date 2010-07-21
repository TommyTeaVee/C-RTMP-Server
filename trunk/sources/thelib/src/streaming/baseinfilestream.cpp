/* 
 *  Copyright (c) 2010,
 *  Gavriloaie Eugen-Andrei (shiretu@gmail.com)
 *
 *  This file is part of crtmpserver.
 *  crtmpserver is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  crtmpserver is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with crtmpserver.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "streaming/baseinfilestream.h"
#include "streaming/basestream.h"
#include "streaming/baseoutstream.h"
#include "streaming/streamstypes.h"
#include "protocols/baseprotocol.h"
#include "mediaformats/basemediadocument.h"
#include "mediaformats/flv/flvdocument.h"
#include "mediaformats/mp3/mp3document.h"
#include "mediaformats/mp4/mp4document.h"

#ifndef HAS_MMAP
map<string, pair<uint32_t, File *> > BaseInFileStream::_fileCache;
#endif /* HAS_MMAP */

BaseInFileStream::InFileStreamTimer::InFileStreamTimer(BaseInFileStream *pInFileStream) {
	_pInFileStream = pInFileStream;
}

BaseInFileStream::InFileStreamTimer::~InFileStreamTimer() {

}

void BaseInFileStream::InFileStreamTimer::ResetStream() {
	_pInFileStream = NULL;
}

bool BaseInFileStream::InFileStreamTimer::TimePeriodElapsed() {
	if (_pInFileStream != NULL)
		_pInFileStream->ReadyForSend();
	return true;
}

BaseInFileStream::BaseInFileStream(BaseProtocol *pProtocol,
		StreamsManager *pStreamsManager, uint64_t type, string name)
: BaseInStream(pProtocol, pStreamsManager, type, name) {
	if (!TAG_KIND_OF(type, ST_IN_FILE)) {
		ASSERT("Incorrect stream type. Wanted a stream type in class %s and got %s",
				STR(tagToString(ST_IN_FILE)), STR(tagToString(type)));
	}
	_pTimer = NULL;
	_pSeekFile = NULL;
	_pFile = NULL;

	//frame info
	_totalFrames = 0;
	_currentFrameIndex = 0;
	memset(&_currentFrame, 0, sizeof (MediaFrame));

	//timing info
	_totalSentTime = 0;
	_totalSentTimeBase = 0;
	_startFeedingTime = 0;

	//buffering info
	_clientSideBufferLength = 0;

	//current state info
	_paused = true;
}

BaseInFileStream::~BaseInFileStream() {
	if (_pTimer != NULL) {
		_pTimer->ResetStream();
		_pTimer->EnqueueForDelete();
		_pTimer = NULL;
	}
	ReleaseFile(_pSeekFile);
	ReleaseFile(_pFile);
}

StreamCapabilities * BaseInFileStream::GetCapabilities() {
	NYI;
	return NULL;
}

bool BaseInFileStream::ResolveCompleteMetadata(Variant &metaData) {
	//1. Create the document
	BaseMediaDocument *pDocument = NULL;
	if (false) {

	}
#ifdef HAS_MEDIA_FLV
	else if (metaData[META_MEDIA_TYPE] == MEDIA_TYPE_FLV ||
			metaData[META_MEDIA_TYPE] == MEDIA_TYPE_LIVE_OR_FLV) {
		pDocument = new FLVDocument(metaData);
	}
#endif /* HAS_MEDIA_FLV */
#ifdef HAS_MEDIA_MP3
	else if (metaData[META_MEDIA_TYPE] == MEDIA_TYPE_MP3) {
		pDocument = new MP3Document(metaData);
	}
#endif /* HAS_MEDIA_MP3 */
#ifdef HAS_MEDIA_MP4
	else if (metaData[META_MEDIA_TYPE] == MEDIA_TYPE_MP4
			|| metaData[META_MEDIA_TYPE] == MEDIA_TYPE_M4A
			|| metaData[META_MEDIA_TYPE] == MEDIA_TYPE_M4V
			|| metaData[META_MEDIA_TYPE] == MEDIA_TYPE_MOV
			|| metaData[META_MEDIA_TYPE] == MEDIA_TYPE_F4V) {
		pDocument = new MP4Document(metaData);
	}
#endif /* HAS_MEDIA_MP4 */
	else {
		FATAL("File type not supported yet. Partial metadata:\n%s",
				STR(metaData.ToString()));
		return false;
	}

	//2. Process the document
	if (!pDocument->Process()) {
		FATAL("Unable to process document");
		delete pDocument;
		MoveFile(metaData[META_SERVER_FULL_PATH],
				(string) metaData[META_SERVER_FULL_PATH] + ".bad");
		return false;
	}

	//3. Get the medatada
	metaData = pDocument->GetMetadata();

	//4. cleanup
	delete pDocument;

	//5. Done
	return true;
}

bool BaseInFileStream::Initialize(int32_t clientSideBufferLength) {
	//1. Check to see if we have an universal seeking file
	string seekFilePath = GetName() + "."MEDIA_TYPE_SEEK;
	if (!fileExists(seekFilePath)) {
		Variant temp;
		temp[META_SERVER_FULL_PATH] = GetName();
		if (!ResolveCompleteMetadata(temp)) {
			FATAL("Unable to generate metadata");
			return false;
		}
	}

	//2. Open the seek file
	_pSeekFile = GetFile(seekFilePath, 128 * 1024);
	if (_pSeekFile == NULL) {
		FATAL("Unable to open seeking file %s", STR(seekFilePath));
		return false;
	}

	//3. Compute the optimal window size by reading the biggest frame size
	//from the seek file.
	if (!_pSeekFile->SeekTo(_pSeekFile->Size() - 8)) {
		FATAL("Unable to seek to %lld position", _pSeekFile->Cursor() - 8);
		return false;
	}
	uint64_t maxFrameSize = 0;
	if (!_pSeekFile->ReadUI64(&maxFrameSize, false)) {
		FATAL("Unable to read max frame size");
		return false;
	}
	if (!_pSeekFile->SeekBegin()) {
		FATAL("Unable to seek to beginning of the file");
		return false;
	}

	//3. Open the media file
	uint32_t windowSize = maxFrameSize * 16;
	windowSize = windowSize < 65536 ? 65536 : windowSize;
	windowSize = (windowSize > (1024 * 1024)) ? (windowSize / 2) : windowSize;
	_pFile = GetFile(GetName(), windowSize);
	if (_pFile == NULL) {
		FATAL("Unable to initialize file");
		return false;
	}

	//4. Read the frames count from the file
	_pSeekFile->SeekBegin();
	if (!_pSeekFile->ReadUI32(&_totalFrames, false)) {
		FATAL("Unable to read the frames count");
		return false;
	}
	//FINEST("_totalFrames: %u", _totalFrames);

	//5. Set the client side buffer length
	_clientSideBufferLength = clientSideBufferLength;

	//6. Create the timer
	_pTimer = new InFileStreamTimer(this);
	_pTimer->EnqueueForTimeEvent(_clientSideBufferLength - _clientSideBufferLength / 3);

	//7. Done
	return true;
}

bool BaseInFileStream::SignalPlay(double &absoluteTimestamp, double &length) {
	//0. fix absoluteTimestamp and length
	absoluteTimestamp = absoluteTimestamp < 0 ? 0 : absoluteTimestamp;
	//TODO: implement the limit playback
	//length = length < 0 ? _totalDuration : length;

	//1. Seek to the correct point
	if (!InternalSeek(absoluteTimestamp)) {
		FATAL("Unable to seek to %.02f", absoluteTimestamp);
		return false;
	}

	//2. Put the stream in active mode
	_paused = false;

	//3. Start the feed reaction
	ReadyForSend();

	//4. Done
	return true;
}

bool BaseInFileStream::SignalPause() {
	//1. Is this already paused
	if (_paused)
		return true;

	//2. Put the stream in paused mode
	_paused = true;

	//3. Done
	return true;
}

bool BaseInFileStream::SignalResume() {
	//1. Is this already active
	if (!_paused)
		return true;

	//2. Put the stream in active mode
	_paused = false;

	//3. Start the feed reaction
	ReadyForSend();

	//5. Done
	return true;
}

bool BaseInFileStream::SignalSeek(double &absoluteTimestamp) {
	//1. Seek to the correct point
	if (!InternalSeek(absoluteTimestamp)) {
		FATAL("Unable to seek to %.02f", absoluteTimestamp);
		return false;
	}

	//2. If the stream is active, re-initiate the feed reaction
	if (!_paused)
		ReadyForSend();

	//3. Done
	return true;
}

bool BaseInFileStream::SignalStop() {
	//1. Is this already paused
	if (_paused)
		return true;

	//2. Put the stream in paused mode
	_paused = true;

	//3. Done
	return true;
}

void BaseInFileStream::ReadyForSend() {
	if (!Feed()) {
		FATAL("Feed failed");
		if (_pOutStreams != NULL)
			_pOutStreams->info->EnqueueForDelete();
	}
}

bool BaseInFileStream::InternalSeek(double &absoluteTimestamp) {
	//1. Switch to millisecond->FrameIndex table
	if (!_pSeekFile->SeekTo(4 + _totalFrames * sizeof (MediaFrame))) {
		FATAL("Failed to seek to ms->FrameIndex table");
		return false;
	}

	//2. Read the sampling rate
	uint32_t samplingRate;
	if (!_pSeekFile->ReadUI32(&samplingRate, false)) {
		FATAL("Unable to read the frames per second");
		return false;
	}

	//3. compute the index in the time2frameindex
	uint32_t tableIndex = (uint32_t) (absoluteTimestamp / samplingRate);

	//4. Seek to that corresponding index
	_pSeekFile->SeekAhead(tableIndex * 4);

	//5. Read the frame index
	uint32_t frameIndex;
	if (!_pSeekFile->ReadUI32(&frameIndex, false)) {
		FATAL("Unable to read frame index");
		return false;
	}

	//7. Position the seek file to that particular frame
	if (!_pSeekFile->SeekTo(4 + frameIndex * sizeof (MediaFrame))) {
		FATAL("Unablt to seek inside seek file");
		return false;
	}

	//8. Read the frame
	if (!_pSeekFile->ReadBuffer((uint8_t *) & _currentFrame, sizeof (MediaFrame))) {
		FATAL("Unable to read frame from seeking file");
		return false;
	}

	//9. update the stream counters
	_startFeedingTime = time(NULL);
	_totalSentTime = 0;
	_currentFrameIndex = frameIndex;
	_totalSentTimeBase = (uint32_t) (_currentFrame.absoluteTime / 1000);
	absoluteTimestamp = _currentFrame.absoluteTime;

	//10. Go back on the frame of interest
	if (!_pSeekFile->SeekTo(4 + frameIndex * sizeof (MediaFrame))) {
		FATAL("Unablt to seek inside seek file");
		return false;
	}

	//11. Done
	return true;
}

bool BaseInFileStream::Feed() {
	//1. Are we in paused state?
	if (_paused)
		return true;

	//2. Determine if the client has enough data on the buffer and continue
	//or stay put
	uint32_t elapsedTime = (uint32_t) (time(NULL) - _startFeedingTime);
	//    FINEST("tst: %d; et: %d; diff: %d",
	//            _totalSentTime, elapsedTime,
	//            (int32_t) _totalSentTime - (int32_t) elapsedTime);
	if ((int32_t) _totalSentTime - (int32_t) elapsedTime >= _clientSideBufferLength) {
		return true;
	}

	//3. Test to see if we have sent the last frame
	if (_currentFrameIndex >= _totalFrames) {
		FINEST("Done streaming file");
		_pOutStreams->info->SignalStreamCompleted();
		_paused = true;
		return true;
	}

	//4. Read the current frame from the seeking file
	if (!_pSeekFile->SeekTo(4 + _currentFrameIndex * sizeof (MediaFrame))) {
		FATAL("Unablt to seek inside seek file");
		return false;
	}
	if (!_pSeekFile->ReadBuffer((uint8_t *) & _currentFrame, sizeof (_currentFrame))) {
		FATAL("Unable to read frame from seeking file");
		return false;
	}

	//5. Skip on metadata
	if (_currentFrame.type == MEDIAFRAME_TYPE_DATA) {
		_currentFrameIndex++;
		return Feed();
	}

	//6. get our hands on the correct buffer, depending on the frame type: audio or video
	IOBuffer &buffer = _currentFrame.type == MEDIAFRAME_TYPE_AUDIO ? _audioBuffer : _videoBuffer;

	//7. Build the frame
	if (!BuildFrame(_pFile, _currentFrame, buffer)) {
		FATAL("Unable to build the frame");
		return false;
	}

	//8. Compute the timestamp
	_totalSentTime = (uint32_t) (_currentFrame.absoluteTime / 1000) - _totalSentTimeBase;

	//9. Do the feedeng
	if (!_pOutStreams->info->FeedData(
			GETIBPOINTER(buffer), //pData
			GETAVAILABLEBYTESCOUNT(buffer), //dataLength
			0, //processedLength
			GETAVAILABLEBYTESCOUNT(buffer), //totalLength
			(uint32_t) _currentFrame.absoluteTime, //absoluteTimestamp
			_currentFrame.type == MEDIAFRAME_TYPE_AUDIO //isAudio
			)) {
		FATAL("Unable to feed audio data");
		return false;
	}

	//10. Discard the data
	buffer.IgnoreAll();


	//11. Increment the frame index
	_currentFrameIndex++;

	//12. Done
	return true;
}

#ifdef HAS_MMAP

MmapFile* BaseInFileStream::GetFile(string filePath, uint32_t windowSize) {
	if (windowSize == 0)
		windowSize = 131072;
	MmapFile *pResult = NULL;
	pResult = new MmapFile();
	if (!pResult->Initialize(filePath, windowSize)) {
		delete pResult;
		return NULL;
	}
	return pResult;
}

void BaseInFileStream::ReleaseFile(MmapFile *pFile) {
	if (pFile == NULL)
		return;
	delete pFile;
}

#else

File* BaseInFileStream::GetFile(string filePath, uint32_t windowSize) {
	File *pResult = NULL;
	if (!MAP_HAS1(_fileCache, filePath)) {
		pResult = new File();
		if (!pResult->Initialize(filePath)) {
			delete pResult;
			return NULL;
		}
		_fileCache[filePath] = pair<uint32_t, File *>(1, pResult);
	} else {
		pResult = _fileCache[filePath].second;
		_fileCache[filePath].first++;
		//FINEST("+Count: %d", _fileCache[filePath].first);
	}
	return pResult;
}

void BaseInFileStream::ReleaseFile(File *pFile) {
	if (pFile == NULL)
		return;
	if (!MAP_HAS1(_fileCache, pFile->GetPath())) {
		WARN("You tryed to release a non-cached file: %s", STR(pFile->GetPath()));
		return;
	}
	_fileCache[pFile->GetPath()].first--;
	if (_fileCache[pFile->GetPath()].first == 0) {
		_fileCache.erase(pFile->GetPath());
		delete pFile;
	}
}
#endif /* HAS_MMAP */
