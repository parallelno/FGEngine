//
// Created by parallelno on 04.04.2016.
//

#include "Asset.h"

Asset::Asset(android_app* pApplication, const char*  pPath):
		mPath(pPath),
		mAssetManager(pApplication->activity->assetManager),
		mAsset(NULL) {
}
bool Asset::open() {
	mAsset = AAssetManager_open(mAssetManager, mPath, AASSET_MODE_UNKNOWN);
	//mLength = AAsset_getLength(mAsset);
	return (mAsset != NULL) ? true : false;
}
void Asset::close() {
	if (mAsset != NULL) {
		AAsset_close(mAsset);
		mAsset = NULL;
	}
}
bool Asset::read(void* pBuffer, size_t pCount) {
	int32_t readCount = AAsset_read(mAsset, pBuffer, pCount);
	return (readCount == pCount) ? true : false;
}
bool Asset::operator==(const Asset& pOther) {
	return !strcmp(mPath, pOther.mPath);
}