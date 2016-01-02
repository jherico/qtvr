//
//  TextureCache.h
//  libraries/model-networking/src
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextureCache_h
#define hifi_TextureCache_h

#include <memory>
#include <QImage>
#include <QMap>
#include <QColor>

#include <DependencyManager.h>
#include <ResourceCache.h>
#include "Forward.h"

class NetworkTexture;
typedef QSharedPointer<NetworkTexture> NetworkTexturePointer;
enum TextureType { DEFAULT_TEXTURE, NORMAL_TEXTURE, BUMP_TEXTURE, SPECULAR_TEXTURE, EMISSIVE_TEXTURE, CUBE_TEXTURE, LIGHTMAP_TEXTURE, CUSTOM_TEXTURE };

/// Stores cached textures, including render-to-texture targets.
class TextureCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    /// Returns the ID of the permutation/normal texture used for Perlin noise shader programs.  This texture
    /// has two lines: the first, a set of random numbers in [0, 255] to be used as permutation offsets, and
    /// the second, a set of random unit vectors to be used as noise gradients.
    const gpu::TexturePointer& getPermutationNormalTexture();

    /// Returns an opaque white texture (useful for a default).
    const gpu::TexturePointer& getWhiteTexture();

    /// Returns an opaque gray texture (useful for a default).
    const gpu::TexturePointer& getGrayTexture();

    /// Returns the a pale blue texture (useful for a normal map).
    const gpu::TexturePointer& getBlueTexture();

    /// Returns the a black texture (useful for a default).
    const gpu::TexturePointer& getBlackTexture();

    // Returns a map used to compress the normals through a fitting scale algorithm
    const gpu::TexturePointer& getNormalFittingTexture();

    /// Returns a texture version of an image file
    static gpu::TexturePointer getImageTexture(const QString& path);

    /// Loads a texture from the specified URL.
    NetworkTexturePointer getTexture(const QUrl& url, TextureType type = DEFAULT_TEXTURE,
        const QByteArray& content = QByteArray());
    
    typedef gpu::Texture* TextureLoader(const QImage& image, const std::string& srcImageName);
    
    typedef std::function<TextureLoader> TextureLoaderFunc;
    
    NetworkTexturePointer getTexture(const QUrl& url, const TextureLoaderFunc& textureLoader,
                                     const QByteArray& content = QByteArray());
protected:

    virtual QSharedPointer<Resource> createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra);
        
private:
    TextureCache();
    virtual ~TextureCache();
    friend class DilatableNetworkTexture;
 
    gpu::TexturePointer _permutationNormalTexture;
    gpu::TexturePointer _whiteTexture;
    gpu::TexturePointer _grayTexture;
    gpu::TexturePointer _blueTexture;
    gpu::TexturePointer _blackTexture;
    gpu::TexturePointer _normalFittingTexture;

    QHash<QUrl, QWeakPointer<NetworkTexture> > _dilatableNetworkTextures;
};

/// A simple object wrapper for an OpenGL texture.
class Texture {
public:
    friend class TextureCache;
    Texture() {}
    ~Texture() {}

    const gpu::TexturePointer getGPUTexture() const;
    gpu::TextureSourcePointer _textureSource;
};

/// A texture loaded from the network.

class NetworkTexture : public Resource, public Texture {
    Q_OBJECT

public:
    
    typedef TextureCache::TextureLoaderFunc TextureLoaderFunc;
    
    NetworkTexture(const QUrl& url, TextureType type, const QByteArray& content);
    NetworkTexture(const QUrl& url, const TextureLoaderFunc& textureLoader, const QByteArray& content);

    int getOriginalWidth() const { return _originalWidth; }
    int getOriginalHeight() const { return _originalHeight; }
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }
    
    TextureLoaderFunc getTextureLoader() const;
    
protected:

    virtual void downloadFinished(const QByteArray& data) override;
          
    Q_INVOKABLE void loadContent(const QByteArray& content);
    // FIXME: This void* should be a gpu::Texture* but i cannot get it to work for now, moving on...
    Q_INVOKABLE void setImage(const QImage& image, void* texture, int originalWidth, int originalHeight);

    virtual void imageLoaded(const QImage& image);

    TextureType _type;

private:
    TextureLoaderFunc _textureLoader;
    int _originalWidth;
    int _originalHeight;
    int _width;
    int _height;
};

#endif // hifi_TextureCache_h
