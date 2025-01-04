//
// Created by parallelno on 04.04.2016.
//

#ifndef ENGINE_ASSET_H
#define ENGINE_ASSET_H
extern "C"{
	#include <sys/stat.h>
}
#include "common.hpp"

class Asset {
public:
	Asset(android_app* pApplication, const char* pPath);
	const char* getPath() {
		return mPath;
	};
	bool open();
	void close();
	bool read(void* pBuffer, size_t pCount);
	bool operator==(const Asset& pOther);
private:
	const char* mPath;
	AAssetManager* mAssetManager;
	AAsset* mAsset;
	int32_t mLength;

};


#endif //ENGINE_ASSET_H
