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


#include "jniwrapper.h"
#include "api.h"

extern "C" void Java_com_rtmpd_CommandsInterface_EnvRun(
		JNIEnv* pEnv, jobject thiz, jstring host, jint port) {
	EnvRun(pEnv->GetStringUTFChars(host, NULL), (uint16_t) port);
}

extern "C" void Java_com_rtmpd_CommandsInterface_EnvStop(
		JNIEnv* pEnv, jobject thiz) {
	EnvStop();
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_ContextCreate(
		JNIEnv* pEnv, jobject thiz) {
	Variant result = ContextCreate();
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_ContextList(
		JNIEnv* pEnv, jobject thiz) {
	Variant result = ContextList();
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_ContextClose(
		JNIEnv* pEnv, jobject thiz, jint contextId) {
	Variant result = ContextClose((uint32_t) contextId);
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_ContextCloseAll(
		JNIEnv* pEnv, jobject thiz) {
	Variant result = ContextCloseAll();
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_CommandPlay(
		JNIEnv* pEnv, jobject thiz, jint contextId, jstring m3u8Uri,
		jstring httpSessionId, jstring keyPassword) {
	Variant result = CommandPlay(
			(uint32_t) contextId,
			pEnv->GetStringUTFChars(m3u8Uri, NULL),
			pEnv->GetStringUTFChars(httpSessionId, NULL),
			pEnv->GetStringUTFChars(keyPassword, NULL));
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_CommandPause(
		JNIEnv* pEnv, jobject thiz, jint contextId) {
	Variant result = CommandPause((uint32_t) contextId);
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_CommandResume(
		JNIEnv* pEnv, jobject thiz, jint contextId) {
	Variant result = CommandResume((uint32_t) contextId);
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_CommandSelectBandwidth(
		JNIEnv* pEnv, jobject thiz, jint contextId, jint bandwidth) {
	Variant result = CommandSelectBandwidth((uint32_t) contextId,
			(uint32_t) bandwidth);
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_CommandSeek(
		JNIEnv* pEnv, jobject thiz, jint contextId, jdouble value) {
	Variant result = CommandSeek((uint32_t) contextId, (double) value);
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_CommandSelectAVChannels(
		JNIEnv* pEnv, jobject thiz, jint contextId, jint audio, jint video) {
	Variant result = CommandSelectAVChannels((uint32_t) contextId,
			(uint32_t) audio, (uint32_t) video);
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_InfoListStreams(
		JNIEnv* pEnv, jobject thiz, jint contextId) {
	Variant result = InfoListStreams((uint32_t) contextId);
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_InfoListAllStreams(
		JNIEnv* pEnv, jobject thiz) {
	Variant result = InfoListAllStreams();
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_InfoBandwidth(
		JNIEnv* pEnv, jobject thiz, jint contextId) {
	Variant result = InfoBandwidth((uint32_t) contextId);
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_InfoAllBandwidth(
		JNIEnv* pEnv, jobject thiz) {
	Variant result = InfoAllBandwidth();
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_InfoPlayback(
		JNIEnv* pEnv, jobject thiz, jint contextId) {
	Variant result = InfoPlayback((uint32_t) contextId);
	return VariantToJObject(result, pEnv);
}

extern "C" jobject Java_com_rtmpd_CommandsInterface_InfoAllPlayback(
		JNIEnv* pEnv, jobject thiz) {
	Variant result = InfoAllPlayback();
	return VariantToJObject(result, pEnv);
}

jobject VariantToJObject(Variant &value, JNIEnv* pEnv) {
	switch ((VariantType) value) {
		case V_NULL:
		case V_UNDEFINED:
		{
			return NULL;
		}
		case V_BOOL:
		{
			jclass clazz = pEnv->FindClass("java/lang/Boolean");
			jmethodID constructor = pEnv->GetMethodID(clazz, "<init>", "(Z)V");
			return pEnv->NewObject(clazz, constructor, (bool)value);
		}
		case V_INT8:
		case V_INT16:
		case V_INT32:
		case V_UINT8:
		case V_UINT16:
		{
			jclass clazz = pEnv->FindClass("java/lang/Integer");
			jmethodID constructor = pEnv->GetMethodID(clazz, "<init>", "(I)V");
			return pEnv->NewObject(clazz, constructor, (int32_t) value);
		}
		case V_INT64:
		case V_UINT32:
		{
			jclass clazz = pEnv->FindClass("java/lang/Long");
			jmethodID constructor = pEnv->GetMethodID(clazz, "<init>", "(J)V");
			return pEnv->NewObject(clazz, constructor, (int64_t) value);
		}
		case V_UINT64:
		{
			NYI;
			return NULL;
		}
		case V_DOUBLE:
		{
			jclass clazz = pEnv->FindClass("java/lang/Double");
			jmethodID constructor = pEnv->GetMethodID(clazz, "<init>", "(D)V");
			return pEnv->NewObject(clazz, constructor, (double) value);
		}
		case V_TIMESTAMP:
		case V_DATE:
		case V_TIME:
		{
			NYI;
			return NULL;
		}
		case V_BYTEARRAY:
		{
			NYI;
			return NULL;
		}
		case V_STRING:
		{
			return pEnv->NewStringUTF(STR(value));
		}
		case V_TYPED_MAP:
		{
			NYI;
			return NULL;
		}
		case V_MAP:
		{
			jclass mapClazz = pEnv->FindClass("java/util/HashMap");
			jmethodID mapConstructor = pEnv->GetMethodID(mapClazz, "<init>", "()V");
			jmethodID mapPut = pEnv->GetMethodID(mapClazz, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

			jclass longClazz = pEnv->FindClass("java/lang/Long");
			jmethodID longConstructor = pEnv->GetMethodID(longClazz, "<init>", "(J)V");

			jobject result = pEnv->NewObject(mapClazz, mapConstructor);

			FOR_MAP(value, string, Variant, i) {
				jobject key = NULL;
				if (MAP_KEY(i).find("__index__value__") == 0) {
					key = pEnv->NewObject(longClazz, longConstructor,
							(int64_t) atol(STR(MAP_KEY(i).substr(16))));
				} else {
					key = pEnv->NewStringUTF(STR(MAP_KEY(i)));
				}
				jobject value = VariantToJObject(MAP_VAL(i), pEnv);
				pEnv->CallObjectMethod(result, mapPut, key, value);
			}

			return result;
		}
		default:
		{
			FATAL("Invalid variant type: %d", (VariantType) value);
			return NULL;
		}
	}
}