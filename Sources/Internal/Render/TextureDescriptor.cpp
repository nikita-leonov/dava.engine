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


#include "Render/TextureDescriptor.h"
#include "FileSystem/Logger.h"
#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/DynamicMemoryFile.h"
#include "Render/Texture.h"
#include "Utils/Utils.h"

#include "Render/GPUFamilyDescriptor.h"

#include "Utils/CRC32.h"

namespace DAVA
{
    
void TextureDescriptor::TextureSettings::SetDefaultValues()
{
    wrapModeS = Texture::WRAP_REPEAT;
    wrapModeT = Texture::WRAP_REPEAT;
    
    generateMipMaps = OPTION_ENABLED;
    
    minFilter = Texture::FILTER_LINEAR_MIPMAP_LINEAR;
    magFilter = Texture::FILTER_LINEAR;
}
    
    
void TextureDescriptor::Compression::Clear()
{
    format = FORMAT_INVALID;
	sourceFileCrc = 0;
    convertedFileCrc = 0;

    compressToWidth = 0;
    compressToHeight = 0;
}
    
    
TextureDescriptor::TextureDescriptor()
{
    isCompressedFile = false;
    InitializeValues();
}

TextureDescriptor::~TextureDescriptor()
{
}

TextureDescriptor *TextureDescriptor::CreateFromFile(const FilePath &filePathname)
{
	if(filePathname.IsEmpty() || filePathname.GetType() == FilePath::PATH_IN_MEMORY)
		return NULL;

    FilePath descriptorPathname = GetDescriptorPathname(filePathname);
    TextureDescriptor *descriptor = new TextureDescriptor();
    bool loaded = descriptor->Load(descriptorPathname);
    if(!loaded)
    {
        Logger::Error("[TextureDescriptor::CreateFromFile(]: there are no descriptor file (%s).", descriptorPathname.GetAbsolutePathname().c_str());
        SafeRelease(descriptor);
        return NULL;
    }
    
    return descriptor;
}
    
    
void TextureDescriptor::InitializeValues()
{
    SetDefaultValues();
    
    for(int32 i = 0; i < GPU_FAMILY_COUNT; ++i)
    {
        compression[i].Clear();
    }
    
    exportedAsGpuFamily = GPU_UNKNOWN;
    exportedAsPixelFormat = FORMAT_INVALID;
	faceDescription = 0;
}
    
void TextureDescriptor::SetDefaultValues()
{
    settings.SetDefaultValues();
}
    
    
bool TextureDescriptor::IsCompressedTextureActual(eGPUFamily forGPU) const
{
    const Compression *compression = GetCompressionParams(forGPU);
	uint32 sourceCRC = ReadSourceCRC();
    uint32 convertedCRC = ReadConvertedCRC(forGPU);
    
	return ((compression->sourceFileCrc == sourceCRC) && (compression->convertedFileCrc == convertedCRC));
}
    
bool TextureDescriptor::UpdateCrcForFormat(eGPUFamily forGPU) const
{
    bool wasUpdated = false;
    const Compression *compression = GetCompressionParams(forGPU);

	uint32 sourceCRC = ReadSourceCRC();
	if(compression->sourceFileCrc != sourceCRC)
	{
		compression->sourceFileCrc = sourceCRC;
		wasUpdated = true;
	}
    
    uint32 convertedCRC = ReadConvertedCRC(forGPU);
	if(compression->convertedFileCrc != convertedCRC)
	{
		compression->convertedFileCrc = convertedCRC;
		wasUpdated = true;
	}
    
    return wasUpdated;
}
    
bool TextureDescriptor::Load(const FilePath &filePathname)
{
    File *file = File::Create(filePathname, File::READ | File::OPEN);
    if(!file)
    {
        Logger::Error("[TextureDescriptor::Load] Can't open file: %s", filePathname.GetAbsolutePathname().c_str());
        return false;
    }
    
    pathname = filePathname;

    int32 signature;
    file->Read(&signature, sizeof(signature));

    int8 version = 0;
    file->Read(&version, sizeof(version));
    if(version != CURRENT_VERSION)
    {
        ConvertToCurrentVersion(version, signature, file);
        SafeRelease(file);
        return true;
    }
    
    isCompressedFile = (COMPRESSED_FILE == signature);
    if(isCompressedFile)
    {
        LoadCompressed(file);
    }
    else if(NOTCOMPRESSED_FILE == signature)
    {
        LoadNotCompressed(file);
    }
    else
    {
        Logger::Error("[TextureDescriptor::Load] Wrong descriptor file: %s", filePathname.GetAbsolutePathname().c_str());
        SafeRelease(file);
        return false;
    }
    
	file->Read(&faceDescription, sizeof(faceDescription));
	
    SafeRelease(file);
    
    return true;
}

void TextureDescriptor::Save() const
{
    DVASSERT_MSG(!pathname.IsEmpty(), "Can use this method only after calling Load()");
    Save(pathname);
}
    
void TextureDescriptor::Save(const FilePath &filePathname) const
{
    File *file = File::Create(filePathname, File::WRITE | File::OPEN | File::CREATE);
    if(!file)
    {
        Logger::Error("[TextureDescriptor::Save] Can't open file: %s", filePathname.GetAbsolutePathname().c_str());
        return;
    }
    
    int32 signature = NOTCOMPRESSED_FILE;
    file->Write(&signature, sizeof(signature));
    
    int8 version = CURRENT_VERSION;
    file->Write(&version, sizeof(version));
    
    WriteGeneralSettings(file);
    
    //Compression
    for(int32 i = 0; i < GPU_FAMILY_COUNT; ++i)
    {
        WriteCompression(file, compression[i]);
    }
	
	file->Write(&faceDescription, sizeof(faceDescription));
    
    SafeRelease(file);
}
  
void TextureDescriptor::Export(const FilePath &filePathname)
{
    File *file = File::Create(filePathname, File::WRITE | File::OPEN | File::CREATE);
    if(!file)
    {
        Logger::Error("[TextureDescriptor::Export] Can't open file: %s", filePathname.GetAbsolutePathname().c_str());
        return;
    }

    int32 signature = COMPRESSED_FILE;
    file->Write(&signature, sizeof(signature));
    
    int8 version = CURRENT_VERSION;
    file->Write(&version, sizeof(version));

    WriteGeneralSettings(file);
    file->Write(&exportedAsGpuFamily, sizeof(exportedAsGpuFamily));
    file->Write(&exportedAsPixelFormat, sizeof(exportedAsPixelFormat));
	
	file->Write(&faceDescription, sizeof(faceDescription));

    SafeRelease(file);
}
    
void TextureDescriptor::ConvertToCurrentVersion(int8 version, int32 signature, DAVA::File *file)
{
//    Logger::Info("[TextureDescriptor::ConvertToCurrentVersion] (%s) from version %d", pathname.c_str(), version);
    
    if(version == 2)
    {
        LoadVersion2(signature, file);
    }
	else if(version == 3)
	{
		LoadVersion3(signature, file);
	}
	else if(version == 4)
	{
		LoadVersion4(signature, file);
	}
    else if(version == 5)
    {
        LoadVersion5(signature, file);
    }
	else if(version == 6)
	{
		LoadVersion6(signature, file);
	}
}
    
void TextureDescriptor::LoadVersion2(int32 signature, DAVA::File *file)
{
    Logger::Warning("[TextureDescriptor::LoadVersion2] function will be deleted");
    
    file->Read(&settings.wrapModeS, sizeof(settings.wrapModeS));
    file->Read(&settings.wrapModeT, sizeof(settings.wrapModeT));
    file->Read(&settings.generateMipMaps, sizeof(settings.generateMipMaps));
    
    if(signature == COMPRESSED_FILE)
    {
		file->Read(&exportedAsGpuFamily, sizeof(exportedAsGpuFamily));
    }
    else if(signature == NOTCOMPRESSED_FILE)
    {
        ReadCompressionWithDateOld(file, compression[GPU_POWERVR_IOS]);
        ReadCompressionWithDateOld(file, compression[GPU_TEGRA]);
    }
}
 
void TextureDescriptor::LoadVersion3(int32 signature, DAVA::File *file)
{
    Logger::Warning("[TextureDescriptor::LoadVersion3] function will be deleted");
    
    file->Read(&settings.wrapModeS, sizeof(settings.wrapModeS));
    file->Read(&settings.wrapModeT, sizeof(settings.wrapModeT));
    file->Read(&settings.generateMipMaps, sizeof(settings.generateMipMaps));
    file->Read(&settings.minFilter, sizeof(settings.minFilter));
    file->Read(&settings.magFilter, sizeof(settings.magFilter));
    
	if(signature == COMPRESSED_FILE)
	{
		file->Read(&exportedAsGpuFamily, sizeof(exportedAsGpuFamily));
	}
	else if(signature == NOTCOMPRESSED_FILE)
	{
        ReadCompressionWithDateOld(file, compression[GPU_POWERVR_IOS]);
        ReadCompressionWithDateOld(file, compression[GPU_TEGRA]);
	}
}

void TextureDescriptor::LoadVersion4(int32 signature, DAVA::File *file)
{
    Logger::Warning("[TextureDescriptor::LoadVersion4] function will be deleted");
    
    file->Read(&settings.wrapModeS, sizeof(settings.wrapModeS));
    file->Read(&settings.wrapModeT, sizeof(settings.wrapModeT));
    file->Read(&settings.generateMipMaps, sizeof(settings.generateMipMaps));
    file->Read(&settings.minFilter, sizeof(settings.minFilter));
    file->Read(&settings.magFilter, sizeof(settings.magFilter));
    
	if(signature == COMPRESSED_FILE)
	{
		file->Read(&exportedAsGpuFamily, sizeof(exportedAsGpuFamily));
	}
	else if(signature == NOTCOMPRESSED_FILE)
	{
		ReadCompressionWith16CRCOld(file, compression[GPU_POWERVR_IOS]);
		ReadCompressionWith16CRCOld(file, compression[GPU_TEGRA]);
	}
}

void TextureDescriptor::LoadVersion5(int32 signature, DAVA::File *file)
{
    file->Read(&settings.wrapModeS, sizeof(settings.wrapModeS));
    file->Read(&settings.wrapModeT, sizeof(settings.wrapModeT));
    file->Read(&settings.generateMipMaps, sizeof(settings.generateMipMaps));
    file->Read(&settings.minFilter, sizeof(settings.minFilter));
    file->Read(&settings.magFilter, sizeof(settings.magFilter));

    if(signature == COMPRESSED_FILE)
	{
		// file->Read(&exportedAsGpuFamily, sizeof(exportedAsGpuFamily));
	}
	else if(signature == NOTCOMPRESSED_FILE)
	{
        int8 format;
        file->Read(&format, sizeof(format));

		if(format == FORMAT_ETC1)
		{
			Logger::Warning("[TextureDescriptor::LoadVersion5] format for pvr was ETC1");

			compression[GPU_POWERVR_IOS].Clear();

			uint32 dummy32 = 0;

			file->Read(&dummy32, sizeof(dummy32));
			file->Read(&dummy32, sizeof(dummy32));
			file->Read(&dummy32, sizeof(dummy32));
		}
		else
		{
			compression[GPU_POWERVR_IOS].format = (PixelFormat)format;

			file->Read(&compression[GPU_POWERVR_IOS].compressToWidth, sizeof(compression[GPU_POWERVR_IOS].compressToWidth));
			file->Read(&compression[GPU_POWERVR_IOS].compressToHeight, sizeof(compression[GPU_POWERVR_IOS].compressToHeight));
			file->Read(&compression[GPU_POWERVR_IOS].sourceFileCrc, sizeof(compression[GPU_POWERVR_IOS].sourceFileCrc));
		}

        file->Read(&format, sizeof(format));

		if(format == FORMAT_ATC_RGB || format == FORMAT_ATC_RGBA_EXPLICIT_ALPHA || format == FORMAT_ATC_RGBA_INTERPOLATED_ALPHA)
		{
			Logger::Warning("[TextureDescriptor::LoadVersion5] format for dds was ATC_...");

			compression[GPU_TEGRA].Clear();

			uint32 dummy32 = 0;

			file->Read(&dummy32, sizeof(dummy32));
			file->Read(&dummy32, sizeof(dummy32));
			file->Read(&dummy32, sizeof(dummy32));
		}
		else
		{
			compression[GPU_TEGRA].format = (PixelFormat)format;

			file->Read(&compression[GPU_TEGRA].compressToWidth, sizeof(compression[GPU_TEGRA].compressToWidth));
			file->Read(&compression[GPU_TEGRA].compressToHeight, sizeof(compression[GPU_TEGRA].compressToHeight));
			file->Read(&compression[GPU_TEGRA].sourceFileCrc, sizeof(compression[GPU_TEGRA].sourceFileCrc));
		}
	}
}

void TextureDescriptor::LoadVersion6(int32 signature, DAVA::File *file)
{
    file->Read(&settings.wrapModeS, sizeof(settings.wrapModeS));
    file->Read(&settings.wrapModeT, sizeof(settings.wrapModeT));
    file->Read(&settings.generateMipMaps, sizeof(settings.generateMipMaps));
    file->Read(&settings.minFilter, sizeof(settings.minFilter));
    file->Read(&settings.magFilter, sizeof(settings.magFilter));
    
    if(signature == COMPRESSED_FILE)
    {
        file->Read(&exportedAsGpuFamily, sizeof(exportedAsGpuFamily));
        file->Read(&exportedAsPixelFormat, sizeof(exportedAsPixelFormat));
    }
    else if(signature == NOTCOMPRESSED_FILE)
    {
        for(int32 i = 0; i < GPU_FAMILY_COUNT; ++i)
        {
            int8 format;
            file->Read(&format, sizeof(format));
            compression[i].format = (PixelFormat)format;
            
            file->Read(&compression[i].compressToWidth, sizeof(compression[i].compressToWidth));
            file->Read(&compression[i].compressToHeight, sizeof(compression[i].compressToHeight));
            file->Read(&compression[i].sourceFileCrc, sizeof(compression[i].sourceFileCrc));
        }
    }
}
    
void TextureDescriptor::LoadNotCompressed(File *file)
{
    ReadGeneralSettings(file);
    
    for(int32 i = 0; i < GPU_FAMILY_COUNT; ++i)
    {
        ReadCompression(file, compression[i]);
    }
}
    
void TextureDescriptor::LoadCompressed(File *file)
{
    ReadGeneralSettings(file);
    file->Read(&exportedAsGpuFamily, sizeof(exportedAsGpuFamily));
    file->Read(&exportedAsPixelFormat, sizeof(exportedAsPixelFormat));
}

void TextureDescriptor::ReadGeneralSettings(File *file)
{
    file->Read(&settings.wrapModeS, sizeof(settings.wrapModeS));
    file->Read(&settings.wrapModeT, sizeof(settings.wrapModeT));
    file->Read(&settings.generateMipMaps, sizeof(settings.generateMipMaps));
    file->Read(&settings.minFilter, sizeof(settings.minFilter));
    file->Read(&settings.magFilter, sizeof(settings.magFilter));
}
    
void TextureDescriptor::WriteGeneralSettings(File *file) const
{
    file->Write(&settings.wrapModeS, sizeof(settings.wrapModeS));
    file->Write(&settings.wrapModeT, sizeof(settings.wrapModeT));
    file->Write(&settings.generateMipMaps, sizeof(settings.generateMipMaps));
    file->Write(&settings.minFilter, sizeof(settings.minFilter));
    file->Write(&settings.magFilter, sizeof(settings.magFilter));
}

    
void TextureDescriptor::ReadCompression(File *file, Compression &compression)
{
    int8 format;
    file->Read(&format, sizeof(format));
    compression.format = (PixelFormat)format;

    file->Read(&compression.compressToWidth, sizeof(compression.compressToWidth));
    file->Read(&compression.compressToHeight, sizeof(compression.compressToHeight));
	file->Read(&compression.sourceFileCrc, sizeof(compression.sourceFileCrc));
	file->Read(&compression.convertedFileCrc, sizeof(compression.convertedFileCrc));
}

void TextureDescriptor::ReadCompressionWithDateOld( File *file, Compression &compression )
{
	int8 format;
	file->Read(&format, sizeof(format));
	compression.format = (PixelFormat)format;

	file->Read(&compression.compressToWidth, sizeof(compression.compressToWidth));
	file->Read(&compression.compressToHeight, sizeof(compression.compressToHeight));

	// skip modification date
	file->Seek(DATE_BUFFER_SIZE * sizeof(char8), File::SEEK_FROM_CURRENT);

	// skip old crc
	file->Seek((MD5::DIGEST_SIZE*2 + 1) * sizeof(char8), File::SEEK_FROM_CURRENT);
	compression.sourceFileCrc = 0;
}

void TextureDescriptor::ReadCompressionWith16CRCOld( File *file, Compression &compression )
{
	int8 format;
	file->Read(&format, sizeof(format));
	compression.format = (PixelFormat)format;

	file->Read(&compression.compressToWidth, sizeof(compression.compressToWidth));
	file->Read(&compression.compressToHeight, sizeof(compression.compressToHeight));

	// skip old crc
	file->Seek((MD5::DIGEST_SIZE*2 + 1) * sizeof(char8), File::SEEK_FROM_CURRENT);
	compression.sourceFileCrc = 0;
}

void TextureDescriptor::WriteCompression(File *file, const Compression &compression) const
{
    int8 format = compression.format;
    file->Write(&format, sizeof(format));
    file->Write(&compression.compressToWidth, sizeof(compression.compressToWidth));
    file->Write(&compression.compressToHeight, sizeof(compression.compressToHeight));
	file->Write(&compression.sourceFileCrc, sizeof(compression.sourceFileCrc));
	file->Write(&compression.convertedFileCrc, sizeof(compression.convertedFileCrc));
}

bool TextureDescriptor::GetGenerateMipMaps() const
{
    return (OPTION_DISABLED != settings.generateMipMaps);
}
    
    
FilePath TextureDescriptor::GetSourceTexturePathname() const
{
    if(pathname.IsEmpty())
    {
        return FilePath();
    }

    return FilePath::CreateWithNewExtension(pathname, GetSourceTextureExtension());
}

FilePath TextureDescriptor::GetDescriptorPathname(const FilePath &texturePathname)
{
    DVASSERT(!texturePathname.IsEmpty());
    
    if(0 == CompareCaseInsensitive(texturePathname.GetExtension(), GetDescriptorExtension()))
    {
        return texturePathname;
    }
    
    DVASSERT(GPUFamilyDescriptor::GetGPUForPathname(texturePathname) == GPU_UNKNOWN);
    
    return FilePath::CreateWithNewExtension(texturePathname, GetDescriptorExtension());
}


String TextureDescriptor::GetDescriptorExtension()
{
    return String(".tex");
}
    
String TextureDescriptor::GetSourceTextureExtension()
{
    return String(".png");
}
    
    
const TextureDescriptor::Compression * TextureDescriptor::GetCompressionParams(eGPUFamily gpuFamily) const
{
    DVASSERT(gpuFamily < GPU_FAMILY_COUNT);
    if(gpuFamily != GPU_UNKNOWN)
    {
        return &compression[gpuFamily];
    }

    return NULL;
}

String TextureDescriptor::GetSupportedTextureExtensions()
{
    return String(".png;.pvr;.dxt;") + TextureDescriptor::GetDescriptorExtension();
}

bool TextureDescriptor::IsCompressedFile() const
{
    return isCompressedFile;
}

bool TextureDescriptor::IsCubeMap() const
{
	return (faceDescription != 0);
}
	
uint32 TextureDescriptor::ReadSourceCRC() const
{
	uint32 crc = 0;

	DAVA::File *f = DAVA::File::Create(GetSourceTexturePathname(), DAVA::File::OPEN | DAVA::File::READ);
	if(NULL != f)
	{
		uint8 buffer[8];

		// Read PNG header
		f->Read(buffer, 8);

		// read chunk header
		while (0 != f->Read(buffer, 8))
		{
			int32 chunk_size = 0;
			chunk_size |= (buffer[0] << 24);
			chunk_size |= (buffer[1] << 16);
			chunk_size |= (buffer[2] << 8);
			chunk_size |= buffer[3];

			// jump thought data
			DVASSERT(chunk_size >= 0);
			f->Seek(chunk_size, File::SEEK_FROM_CURRENT);

			// read crc
			f->Read(buffer, 4);
			crc += ((uint32 *) buffer)[0];
		}

		f->Release();
	}

	return crc;
}
    
uint32 TextureDescriptor::ReadConvertedCRC(eGPUFamily forGPU) const
{
    return CRC32::ForFile(GPUFamilyDescriptor::CreatePathnameForGPU(this, forGPU));
}

    
PixelFormat TextureDescriptor::GetPixelFormatForCompression(eGPUFamily forGPU)
{
	if(forGPU == GPU_UNKNOWN)	
		return FORMAT_INVALID;

    DVASSERT(0 <= forGPU && forGPU < GPU_FAMILY_COUNT);
    return (PixelFormat) compression[forGPU].format;
}

    
    
};
