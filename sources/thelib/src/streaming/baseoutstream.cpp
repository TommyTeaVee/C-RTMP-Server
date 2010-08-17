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


#include "streaming/baseoutstream.h"
#include "streaming/streamstypes.h"
#include "streaming/baseinstream.h"

BaseOutStream::BaseOutStream(BaseProtocol *pProtocol,
		StreamsManager *pStreamsManager, uint64_t type, string name)
: BaseStream(pProtocol, pStreamsManager, type, name) {
	if (!TAG_KIND_OF(type, ST_OUT)) {
		ASSERT("Incorrect stream type. Wanted a stream type in class %s and got %s",
				STR(tagToString(ST_OUT)), STR(tagToString(type)));
	}
	_canCallDetachedFromInStream = true;
	_pInStream = NULL;
}

BaseOutStream::~BaseOutStream() {
	_canCallDetachedFromInStream = false;
	UnLink(true);
}

StreamCapabilities * BaseOutStream::GetCapabilities() {
	if (_pInStream != NULL)
		return _pInStream->GetCapabilities();
	return NULL;
}

void BaseOutStream::ReadyForSend() {
	if (_pInStream != NULL)
		_pInStream->ReadyForSend();
}

bool BaseOutStream::Link(BaseInStream *pInStream, bool reverseLink) {
	if ((!pInStream->IsCompatibleWithType(GetType()))
			|| (!IsCompatibleWithType(pInStream->GetType()))) {
		FATAL("stream type %s not compatible with stream type %s",
				STR(tagToString(GetType())),
				STR(tagToString(pInStream->GetType())));
		return false;
	}
	//    FINEST("BaseOutStream::Link: this: %u; pInStream: %u; reverseLink: %d",
	//            GetUniqueId(), pInStream->GetUniqueId(), reverseLink);
	if (_pInStream != NULL) {
		if (_pInStream->GetUniqueId() == pInStream->GetUniqueId()) {
			WARN("BaseOutStream::Link: This stream is already linked");
			return true;
		}
		FATAL("BaseOutStream::Link: This stream is already linked to stream with unique id %u",
				_pInStream->GetUniqueId());
		return false;
	}
	//FINEST("BaseOutStream::Link: Linking...");
	_pInStream = pInStream;
	if (reverseLink) {
		//FINEST("BaseOutStream::Link: Reverse linking...");
		if (!_pInStream->Link(this, false)) {
			FATAL("BaseOutStream::Link: Unable to reverse link");
			_pInStream = NULL;
			return false;
		}
		//FINEST("BaseOutStream::Link: Done reverse link");
	}
	//FINEST("BaseOutStream::Link: Signal AttachedToInStream");
	SignalAttachedToInStream();
	//FINEST("BaseOutStream::Link: Done");
	return true;
}

bool BaseOutStream::UnLink(bool reverseUnLink) {
	//    FINEST("BaseOutStream::UnLink: this: %u; _pInStream: %u; reverseUnLink: %d",
	//            GetUniqueId(), _pInStream != NULL ? _pInStream->GetUniqueId() : 0,
	//            reverseUnLink);
	if (_pInStream == NULL) {
		WARN("BaseOutStream::UnLink: This stream is not linked");
		return true;
	}
	if (reverseUnLink) {
		//FINEST("BaseOutStream::UnLink: Reverse unLinking...");
		if (!_pInStream->UnLink(this, false)) {
			FATAL("BaseOutStream::UnLink: Unable to reverse unLink");
			//TODO: what are we going to do here???
			NYIA;
		}
		//FINEST("BaseOutStream::UnLink: Done reverse unLink");
	}
	//FINEST("BaseOutStream::UnLink: unLinking...");
	_pInStream = NULL;

	//    FINEST("BaseOutStream::UnLink: _canCallDetachedFromInStream: %d",
	//            _canCallDetachedFromInStream);
	if (_canCallDetachedFromInStream) {
		//FINEST("BaseOutStream::UnLink: Signal DetachedFromInStream");
		SignalDetachedFromInStream();
	}
	//FINEST("BaseOutStream::UnLink: Done");
	return true;
}

bool BaseOutStream::IsLinked() {
	return _pInStream != NULL;
}

BaseInStream *BaseOutStream::GetInStream() {
	return _pInStream;
}

bool BaseOutStream::Play(double absoluteTimestamp, double length) {
	if (_pInStream != NULL) {
		if (!_pInStream->SignalPlay(absoluteTimestamp, length)) {
			FATAL("Unable to signal play");
			return false;
		}
	}
	return SignalPlay(absoluteTimestamp, length);
}

bool BaseOutStream::Pause() {
	if (_pInStream != NULL) {
		if (!_pInStream->SignalPause()) {
			FATAL("Unable to signal pause");
			return false;
		}
	}
	return SignalPause();
}

bool BaseOutStream::Resume() {
	if (_pInStream != NULL) {
		if (!_pInStream->SignalResume()) {
			FATAL("Unable to signal resume");
			return false;
		}
	}
	return SignalResume();
}

bool BaseOutStream::Seek(double absoluteTimestamp) {
	if (!SignalSeek(absoluteTimestamp)) {
		FATAL("Unable to signal seek");
		return false;
	}

	if (_pInStream != NULL) {
		if (!_pInStream->SignalSeek(absoluteTimestamp)) {
			FATAL("Unable to signal seek");
			return false;
		}
	}

	return true;
}

bool BaseOutStream::Stop() {
	if (_pInStream != NULL) {
		if (!_pInStream->SignalStop()) {
			FATAL("Unable to signal stop");
			return false;
		}
	}
	return SignalStop();
}
