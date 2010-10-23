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


#ifndef _BASESTREAM_H
#define	_BASESTREAM_H

#include "common.h"
#include "streaming/streamcapabilities.h"

class StreamsManager;
class BaseProtocol;

class DLLEXP BaseStream {
protected:
	StreamsManager *_pStreamsManager;
	uint64_t _type;
	uint32_t _uniqueId;
	BaseProtocol *_pProtocol;
	string _name;
public:
	BaseStream(BaseProtocol *pProtocol, StreamsManager *pStreamsManager,
			uint64_t type, string name);
	virtual ~BaseStream();

	/*
	 * Returns the stream manager. This is read-only
	 * */
	StreamsManager * GetStreamsManager();

	/*
	 * Returns the stream capabilities. Specifically, codec and codec related info
	 * */
	virtual StreamCapabilities * GetCapabilities() = 0;

	/*
	 * Returns the type of this stream. This is read-only
	 * */
	uint64_t GetType();

	/*
	 * Returns the unique id of this stream. This is read-only
	 * */
	uint32_t GetUniqueId();

	/*
	 * Returns the name of this stream. This is setup-once
	 * */
	string GetName();
	void SetName(string name);

	/*
	 * Returns the protocol that owns this stream.
	 * */
	BaseProtocol * GetProtocol();

	/*
	 * Tells if this stream is enqueued for delete or not based on the pProtocol
	 * */
	virtual bool IsEnqueueForDelete();

	/*
	 * Will enqueue this stream for delete along with his protocol
	 * */
	virtual void EnqueueForDelete();

	/*
	 * This will start the feeding process
	 * absoluteTimestamp - the timestamp where we want to seek
	 *                     before start the feeding process
	 * length - time limit
	 * */
	virtual bool Play(double absoluteTimestamp, double length) = 0;

	/*
	 * This will pause the feeding process
	 * */
	virtual bool Pause() = 0;

	/*
	 * This will resume the feeding process
	 * */
	virtual bool Resume() = 0;

	/*
	 * This will seek to the specified point in time.
	 * */
	virtual bool Seek(double absoluteTimestamp) = 0;

	/*
	 * This will stop the feeding process
	 * */
	virtual bool Stop() = 0;


	/*
	 * Called when a play command was issued
	 * absoluteTimestamp - the timestamp where we want to seek
	 *                     before start the feeding process
	 * */
	virtual bool SignalPlay(double &absoluteTimestamp, double &length) = 0;

	/*
	 * Called when a pasue command was issued
	 * */
	virtual bool SignalPause() = 0;

	/*
	 * Called when a resume command was issued
	 * */
	virtual bool SignalResume() = 0;

	/*
	 * Called when a seek command was issued
	 * absoluteTimestamp - the
	 * */
	virtual bool SignalSeek(double &absoluteTimestamp) = 0;

	/*
	 * Called when a stop command was issued
	 * */
	virtual bool SignalStop() = 0;

	/*
	 * pData - the buffer containing the data
	 * dataLength - the size of pData in bytes
	 * processedLength - if pData is only partial data, this shows the numbers
	 *                   of bytes processed so far, excluding pData
	 * totalLength - if pData is only partial data, this shows the total number
	 *               of bytes inside the current packet
	 * isAudio - true if pData is audio data, false if pData is video data
	 *
	 * Rulles:
	 * dataLength+processedLength<=totalLength
	 * dataLength<=totalLength
	 * processedLength<=totalLength
	 * dataLength!=0
	 * */
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double absoluteTimestamp, bool isAudio) = 0;

	/*
	 * The networking layer signaled the availability for sending data
	 * */
	virtual void ReadyForSend() = 0;

	/*
	 * This is called to ensure that the linking process can be done
	 * type - the target type to which this strem must be linked against
	 * */
	virtual bool IsCompatibleWithType(uint64_t type) = 0;
};

#endif	/* _BASESTREAM_H */
