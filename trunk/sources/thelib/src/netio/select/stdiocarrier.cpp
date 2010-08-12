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


#ifdef NET_SELECT
#include "netio/select/stdiocarrier.h"
#include "netio/select/iohandlermanager.h"
#include "protocols/baseprotocol.h"

#define ENABLE_WRITE_DATA \
if (!_writeDataEnabled) { \
    _writeDataEnabled = true; \
    IOHandlerManager::EnableWriteData(this); \
} \
/*else{ \
    FINEST("Write data already enabled"); \
}*/

#define DISABLE_WRITE_DATA \
if (_writeDataEnabled) { \
    _writeDataEnabled = false; \
    IOHandlerManager::DisableWriteData(this); \
    _pProtocol->ReadyForSend(); \
} \
/*else { \
    FINEST("Write data already disabled"); \
}*/

StdioCarrier *StdioCarrier::_pInstance = NULL;

StdioCarrier::StdioCarrier(BaseProtocol *pProtocol)
: IOHandler(fileno(stdin), fileno(stdout), IOHT_STDIO) {
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

bool StdioCarrier::OnEvent(select_event &event) {
	int32_t recvAmount = 0;

	//3. Do the I/O
	switch (event.type) {
		case SET_READ:
		{
			IOBuffer *pInputBuffer = _pProtocol->GetInputBuffer();
			assert(pInputBuffer != NULL);
			if (!pInputBuffer->ReadFromStdio(_inboundFd,
					FD_READ_CHUNK, recvAmount)) {
				FATAL("Unable to read data");
				return false;
			}

			return _pProtocol->SignalInputData(recvAmount);
		}
		case SET_WRITE:
		{
			IOBuffer *pOutputBuffer = NULL;

			while ((pOutputBuffer = _pProtocol->GetOutputBuffer()) != NULL) {
				//FINEST("Try to send buffer:\n%s", STR(*pOutputBuffer));
				if (!pOutputBuffer->WriteToStdio(_outboundFd,
						FD_WRITE_CHUNK)) {
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
			return true;
		}
		default:
		{
			ASSERT("Invalid state: %d", event.type);
			return false;
		}
	}
}

bool StdioCarrier::SignalOutputData() {
	ENABLE_WRITE_DATA;
	return true;
}

StdioCarrier::operator string() {
	if (_pProtocol != NULL)
		return STR(*_pProtocol);
	return format("IO(%d,%d)", _inboundFd, _outboundFd);
}


#endif /* NET_SELECT */
