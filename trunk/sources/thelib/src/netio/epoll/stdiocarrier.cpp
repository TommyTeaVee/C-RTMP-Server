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


#ifdef NET_EPOLL
#include "netio/epoll/stdiocarrier.h"
#include "netio/epoll/iohandlermanager.h"
#include "protocols/baseprotocol.h"

/*#define ENABLE_WRITE_DATA \
if (!_writeDataEnabled) { \
	_writeDataEnabled = true; \
	IOHandlerManager::EnableWriteData(this); \
}


#define DISABLE_WRITE_DATA \
if (_writeDataEnabled) { \
	_writeDataEnabled = false; \
	IOHandlerManager::DisableWriteData(this); \
	_pProtocol->ReadyForSend(); \
}*/

StdioCarrier *StdioCarrier::_pInstance = NULL;

StdioCarrier::StdioCarrier(BaseProtocol *pProtocol)
: IOHandler(fileno(stdin), fileno(stdout), IOHT_TCP_CARRIER) {
	_pProtocol = pProtocol;
	IOHandlerManager::EnableReadData(this);
	_writeDataEnabled = false;
}

StdioCarrier *StdioCarrier::GetInstance(BaseProtocol *pProtocol) {
	if (_pInstance == NULL) {
		_pInstance = new StdioCarrier(pProtocol);
		return _pInstance;
	}
	assert(_pInstance->_pProtocol != NULL);
	assert(pProtocol != NULL);
	if (_pInstance->_pProtocol->GetId() != pProtocol->GetId()) {
		FATAL("Stdio carrier is already acquired");
		return NULL;
	}
	return _pInstance;
}

StdioCarrier::~StdioCarrier() {
	if (_pProtocol != NULL) {
		_pProtocol->SetIOHandler(NULL);
		delete _pProtocol;
	}
	_pInstance = NULL;
}

void StdioCarrier::ResetProtocol() {
	_pProtocol = NULL;
}

bool StdioCarrier::OnEvent(struct epoll_event &event) {
	//FINEST("Event: %d", event.events);

	//1. Read data
	if ((event.events & EPOLLIN) != 0) {
		IOBuffer *pInputBuffer = _pProtocol->GetInputBuffer();
		assert(pInputBuffer != NULL);
		int32_t recvBytes = 0;
		if (!pInputBuffer->ReadFromStdio(_inboundFd, FD_READ_CHUNK, recvBytes)) {
			FATAL("Unable to read data");
			return false;
		}
		//FINEST("recvBytes: %d; _totalRecveivedBytes: %d", recvBytes, _totalRecveivedBytes);
		if (recvBytes == 0) {
			FATAL("Connection closed");
			return false;
		}

		if (!_pProtocol->SignalInputData(recvBytes)) {
			FATAL("Unable to signal data available");
			return false;
		}
	}

	/*//2. Write data
	if ((event.events & EPOLLOUT) != 0) {
		IOBuffer *pOutputBuffer = NULL;

		while ((pOutputBuffer = _pProtocol->GetOutputBuffer()) != NULL) {
			if (!pOutputBuffer->PutToStdio(_outboundFd, FD_WRITE_CHUNK)) {
				FATAL("Unable to send data");
				IOHandlerManager::EnqueueForDelete(this);
				return false;
			}
			if (GETAVAILABLEBYTESCOUNT(*pOutputBuffer) > 0) {
				ENABLE_WRITE_DATA;
				break;
			}
		}

		if (pOutputBuffer == NULL) {
			DISABLE_WRITE_DATA;
		}
	}*/

	return true;
}

bool StdioCarrier::SignalOutputData() {
	IOBuffer *pOutputBuffer = NULL;

	while ((pOutputBuffer = _pProtocol->GetOutputBuffer()) != NULL) {
		if (!pOutputBuffer->WriteToStdio(_outboundFd, FD_WRITE_CHUNK)) {
			FATAL("Unable to send data");
			IOHandlerManager::EnqueueForDelete(this);
			return false;
		}
	}
	return true;
}

StdioCarrier::operator string() {
	if (_pProtocol != NULL)
		return STR(*_pProtocol);
	return format("IO(%d,%d)", _inboundFd, _outboundFd);
}


#endif /* NET_EPOLL */
