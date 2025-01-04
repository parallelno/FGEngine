//
// Created by parallelno on 05.04.2016.
//

#ifndef ENGINE_ASSETMANAGER_H
#define ENGINE_ASSETMANAGER_H


#include "common.hpp"

class AssetManager {
public:
	AssetManager(android_app* app);
	~AssetManager();

private:
	android_app* app;

};


#endif //ENGINE_ASSETMANAGER_H
