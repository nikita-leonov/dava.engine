/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/



#include "TextureCache.h"
#include "TextureConvertor.h"

#include <QPainter>

TextureCache::TextureCache()
{
    new TextureConvertor();
    
    QObject::connect(TextureConvertor::Instance(), SIGNAL(ReadyOriginal(const DAVA::TextureDescriptor *, const DAVA::Vector<QImage>&)), this, SLOT(ReadyOriginal(const DAVA::TextureDescriptor *, const DAVA::Vector<QImage>&)));
    QObject::connect(TextureConvertor::Instance(), SIGNAL(ReadyConverted(const DAVA::TextureDescriptor *, const DAVA::eGPUFamily, const DAVA::Vector<QImage>&)), this, SLOT(ReadyConverted(const DAVA::TextureDescriptor *, const DAVA::eGPUFamily, const DAVA::Vector<QImage>&)));
}

TextureCache::~TextureCache()
{
    TextureConvertor::Instance()->Release();
}


DAVA::Vector<QImage>  TextureCache::getThumbnail(const DAVA::TextureDescriptor *descriptor)
{
	if(NULL != descriptor && cacheThumbnail.contains(descriptor))
	{
		// update weight for this cached
		cacheThumbnail[descriptor].weight = curThumbnailWeight++;
        return cacheThumbnail[descriptor].images;
	}
    
	return DAVA::Vector<QImage>();
}


DAVA::Vector<QImage> TextureCache::getOriginal(const DAVA::TextureDescriptor *descriptor)
{
	DAVA::Vector<QImage> images;

	if(NULL != descriptor && cacheOriginal.contains(descriptor))
	{
		// update weight for this cached
		cacheOriginal[descriptor].weight = curOriginalWeight++;
		images = cacheOriginal[descriptor].images;
	}

	return images;
}

DAVA::Vector<QImage> TextureCache::getConverted(const DAVA::TextureDescriptor *descriptor, const DAVA::eGPUFamily gpu)
{
	DAVA::Vector<QImage> images;

	if( NULL != descriptor &&
		gpu > DAVA::GPU_UNKNOWN && gpu < DAVA::GPU_FAMILY_COUNT &&
		cacheConverted[gpu].contains(descriptor))
	{
		// update weight for this cached
		cacheConverted[gpu][descriptor].weight = curConvertedWeight[gpu]++;
		images = cacheConverted[gpu][descriptor].images;
	}

	return images;
}


void TextureCache::ReadyOriginal(const DAVA::TextureDescriptor *descriptor, const DAVA::Vector<QImage>& image)
{
    setOriginal(descriptor, image);
    setThumbnail(descriptor, image);
}

void TextureCache::ReadyConverted(const DAVA::TextureDescriptor *descriptor, const DAVA::eGPUFamily gpu, const DAVA::Vector<QImage>& image)
{
    setConverted(descriptor, gpu, image);
}


void TextureCache::setThumbnail(const DAVA::TextureDescriptor *descriptor, const DAVA::Vector<QImage> &images)
{
	if(NULL != descriptor)
	{
        DAVA::Vector<QImage> tmpImages;
		for(size_t i = 0; i < images.size(); ++i)
		{
            QImage img(THUMBNAIL_SIZE, THUMBNAIL_SIZE, QImage::Format_ARGB32);
            img.fill(QColor(Qt::white));
            
            QPainter painter(&img);
            
            QSize imageSize = images[i].rect().size();
            imageSize.scale(THUMBNAIL_SIZE, THUMBNAIL_SIZE, Qt::KeepAspectRatio);
            int x = (THUMBNAIL_SIZE - imageSize.width()) / 2;
            int y = (THUMBNAIL_SIZE - imageSize.height()) / 2;
            painter.drawImage(QRect(QPoint(x, y), imageSize), images[i]);
            
            painter.end();

            tmpImages.push_back(img);
		}
        
        cacheThumbnail[descriptor] = CacheEntity(tmpImages, curThumbnailWeight++);
        ClearCacheTail(cacheThumbnail, curThumbnailWeight, maxThumbnailCount);
        
        emit ThumbnailLoaded(descriptor, cacheThumbnail[descriptor].images);
    }
}

void TextureCache::setOriginal(const DAVA::TextureDescriptor *descriptor, const DAVA::Vector<QImage> & images)
{
	if(NULL != descriptor)
	{
 		DAVA::Vector<QImage> tmpImages;
		for(size_t i = 0; i < images.size(); ++i)
		{
			tmpImages.push_back(images[i]);
		}
		
		cacheOriginal[descriptor] = CacheEntity(tmpImages, curOriginalWeight++);
        ClearCacheTail(cacheOriginal, curOriginalWeight, maxOrigCount);
        
        emit OriginalLoaded(descriptor, cacheOriginal[descriptor].images);
	}
}

void TextureCache::setConverted(const DAVA::TextureDescriptor *descriptor, const DAVA::eGPUFamily gpu, const DAVA::Vector<QImage>& images)
{
	if( NULL != descriptor && 
		gpu > DAVA::GPU_UNKNOWN && gpu < DAVA::GPU_FAMILY_COUNT)
	{
		DAVA::Vector<QImage> tmpImages;
		for(int i = 0; i < (int)images.size(); ++i)
		{
			tmpImages.push_back(images[i]);
		}

		cacheConverted[gpu][descriptor] = CacheEntity(tmpImages, curConvertedWeight[gpu]++);
        ClearCacheTail(cacheConverted[gpu], curConvertedWeight[gpu], maxConvertedCount);
        
        emit ConvertedLoaded(descriptor, gpu, cacheConverted[gpu][descriptor].images);
	}
}

void TextureCache::ClearCacheTail(QMap<const DAVA::TextureDescriptor*, CacheEntity> & cache, const size_t currentWeight, const size_t maxWeight)
{
    if(cache.size() > maxWeight)
    {
        size_t weightToRemove = currentWeight - maxWeight;
 
        QMapIterator<const DAVA::TextureDescriptor*, CacheEntity> iter(cache);
        while(iter.hasNext())
        {
            iter.next();
            if(iter.value().weight < (size_t)weightToRemove)
            {
                cache.remove(iter.key());
            }
        }
    }
}


void TextureCache::clearInsteadThumbnails()
{
	cacheOriginal.clear();
	for(int i = DAVA::GPU_UNKNOWN + 1; i < DAVA::GPU_FAMILY_COUNT; ++i)
	{
		cacheConverted[i].clear();
	}
}

void TextureCache::clearThumbnail(const DAVA::TextureDescriptor *descriptor)
{
	cacheThumbnail.remove(descriptor);
}

void TextureCache::clearOriginal(const DAVA::TextureDescriptor *descriptor)
{
	cacheOriginal.remove(descriptor);
}

void TextureCache::clearConverted(const DAVA::TextureDescriptor *descriptor, const DAVA::eGPUFamily gpu)
{
	if(gpu > DAVA::GPU_UNKNOWN && gpu < DAVA::GPU_FAMILY_COUNT)
	{
		cacheConverted[gpu].remove(descriptor);
	}
}
