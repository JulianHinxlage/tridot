//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Archive.h"
#include "core/Reflection.h"
#include "Serializer.h"
#include "AssetManager.h"
#include "Color.h"
#include "MetaTypes.h"
#include <glm/glm.hpp>

namespace tri {

	Archive::Archive() {
		bytesArchive = nullptr;
		stringArchive = nullptr;
		classArchive = nullptr;
	}

	void Archive::writeBytes(const void* ptr, int bytes) {
		if (bytesArchive) {
			bytesArchive->writeBytes(ptr, bytes);
		}
	}

	void Archive::readBytes(void* ptr, int bytes) {
		if (bytesArchive) {
			bytesArchive->readBytes(ptr, bytes);
		}
	}

	bool Archive::hasDataLeft() {
		if (bytesArchive) {
			return bytesArchive->hasDataLeft();
		}
		return false;
	}

	void Archive::writeClass(const void* ptr, int classId) {
		if (classArchive) {
			classArchive->writeClass(ptr, classId);
		}
		else {
			writeBytes(ptr, Reflection::getDescriptor(classId)->size);
		}
	}

	void Archive::readClass(void* ptr, int classId) {
		if (classArchive) {
			classArchive->readClass(ptr, classId);
		}
		else {
			readBytes(ptr, Reflection::getDescriptor(classId)->size);
		}
	}

	void Archive::writeStr(const std::string& str) {
		if (stringArchive) {
			stringArchive->writeStr(str);
		}
		else {
			writeBytes(str.c_str(), (int)str.size() + 1);
		}
	}

	void Archive::readStr(std::string& str) {
		if (stringArchive) {
			stringArchive->readStr(str);
		}
		else {
			while (true) {
				char c = '\0';
				readBytes(&c, 1);
				if (c != '\0') {
					str.push_back(c);
				}
				else {
					break;
				}
			}
		}
	}

	std::string Archive::readStr() {
		std::string str;
		readStr(str);
		return str;
	}

	void Archive::writeVarInt(const int64_t& value) {
		uint8_t* ptr = (uint8_t*)&value;
		if (value >= 0 && value < (1 << 7)) {
			writeBytes(&ptr[0], 1);
			return;
		}
		
		int readBit = 0;
		int writeBit = 0;
		int zeroBits = 0;
		uint8_t byte = 0;
		for (int i = 0; i < sizeof(value) * 8; i++) {
			uint8_t bit = (ptr[readBit / 8] >> (readBit % 8)) & 1u;
			readBit++;
			if (bit) {
				for (int j = 0; j < zeroBits; j++) {
					if (writeBit >= 7) {
						byte |= (1 << 7);
						writeBytes(&byte, 1);
						byte = 0;
						writeBit = 0;
					}
					
					writeBit++;
				}
				zeroBits = 0;

				if (writeBit >= 7) {
					byte |= (1 << 7);
					writeBytes(&byte, 1);
					byte = 0;
					writeBit = 0;
				}
				byte |= (1 << writeBit);
				writeBit++;
			}
			else {
				zeroBits++;
			}
		}

		if (writeBit >= 1 || zeroBits > 0) {
			writeBytes(&byte, 1);
			byte = 0;
			writeBit = 0;
		}
	}

	void Archive::readVarInt(int64_t& value) {
		int readBit = 0;
		int writeBit = 0;
		uint8_t* ptr = (uint8_t*)&value;
		uint8_t byte = 0;

		while (true) {
			readBytes(&byte, 1);

			for (int i = 0; i < 7; i++) {
				uint8_t bit = (byte >> i) & 1u;

				if (writeBit < sizeof(value) * 8) {
					if (bit) {
						ptr[writeBit / 8] |= (1 << (writeBit % 8));
					}
					else {
						ptr[writeBit / 8] &= ~(1 << (writeBit % 8));
					}
					writeBit++;
				}
			}

			if (!((byte >> 7) & 1u)) {
				break;
			}
		}

	}


	MemoryArchive::MemoryArchive() {
		dataPtr = nullptr;
		dataSize = 0;
		readIndex = 0;
		writeIndex = 0;
	}

	MemoryArchive::MemoryArchive(void* data, int bytes) {
		dataPtr = nullptr;
		dataSize = 0;
		readIndex = 0;
		writeIndex = 0;
		setData(data, bytes);
	}

	void MemoryArchive::writeBytes(const void* ptr, int bytes) {
		int left = dataSize - writeIndex;
		if (bytes > left) {
			reserve(writeIndex + bytes);
		}
		memcpy(dataPtr + writeIndex, ptr, bytes);
		writeIndex += bytes;
	}

	void MemoryArchive::readBytes(void* ptr, int bytes) {
		int left = dataSize - readIndex;
		int min = bytes < left ? bytes : left;
		memcpy(ptr, dataPtr + readIndex, min);
		readIndex += min;
		memset((uint8_t*)ptr + min, 0, bytes - min);
	}

	bool MemoryArchive::hasDataLeft() {
		return readIndex < dataSize;
	}

	void MemoryArchive::skip(int bytes) {
		readIndex += bytes;
	}

	void MemoryArchive::unskip(int bytes) {
		readIndex -= bytes;
	}

	void MemoryArchive::reset() {
		readIndex = 0;
	}

	uint8_t* MemoryArchive::data() {
		return dataPtr + readIndex;
	}

	int MemoryArchive::size() {
		return dataSize - readIndex;
	}

	void MemoryArchive::reserve(int bytes) {
		if (buffer.data() == dataPtr) {
			buffer.resize(bytes);
			dataSize = (int)buffer.size();
			dataPtr = buffer.data();
		}
		else {
			buffer.resize(bytes);
			memcpy(buffer.data(), dataPtr, dataSize);
			dataSize = (int)buffer.size();
			dataPtr = buffer.data();
		}
	}

	void MemoryArchive::setData(void* data, int bytes) {
		dataPtr = (uint8_t*)data;
		dataSize = bytes;
		writeIndex = 0;
		readIndex = 0;
		buffer.clear();
	}

	void MemoryArchive::clear() {
		dataPtr = nullptr;
		buffer.clear();
		dataSize = 0;
		readIndex = 0;
		writeIndex = 0;
	}

	int MemoryArchive::getReadIndex() {
		return readIndex;
	}

	int MemoryArchive::getWriteIndex() {
		return writeIndex;
	}


	class ArchiveBinaryMapper {
	public:
		class Step {
		public:
			bool plain;
			int bytes;
			int offset;
			std::function<void(const void* ptr, Archive& archive)> writeCallback;
			std::function<void(void* ptr, Archive& archive)> readCallback;
		};
		std::vector<Step> steps;

		void read(void* ptr, Archive& archive);
		void write(const void* ptr, Archive& archive);
		void create(int classId, int offset = 0);


		static std::vector<std::shared_ptr<ArchiveBinaryMapper>> archiveBinaryMappers;
		static ArchiveBinaryMapper* getMapper(int classId) {
			if (archiveBinaryMappers.size() <= classId) {
				archiveBinaryMappers.resize(classId + 1);
			}
			if (!archiveBinaryMappers[classId]) {
				archiveBinaryMappers[classId] = std::make_shared<ArchiveBinaryMapper>();
				archiveBinaryMappers[classId]->create(classId);
			}
			return archiveBinaryMappers[classId].get();
		}
	};
	std::vector<std::shared_ptr<ArchiveBinaryMapper>> ArchiveBinaryMapper::archiveBinaryMappers;

	void BinaryArchive::writeClass(const void* ptr, int classId) {
		ArchiveBinaryMapper::getMapper(classId)->write(ptr, *this);
	}

	void BinaryArchive::readClass(void* ptr, int classId) {
		ArchiveBinaryMapper::getMapper(classId)->read(ptr, *this);
	}

	void ArchiveBinaryMapper::read(void* ptr, Archive &archive) {
		for (auto& step : steps) {
			if (step.plain) {
				archive.readBytes((uint8_t*)ptr + step.offset, step.bytes);
			}
			else {
				step.readCallback((uint8_t*)ptr + step.offset, archive);
			}
		}
	}

	void ArchiveBinaryMapper::write(const void* ptr, Archive& archive) {
		for (auto& step : steps) {
			if (step.plain) {
				archive.writeBytes((uint8_t*)ptr + step.offset, step.bytes);
			}
			else {
				step.writeCallback((uint8_t*)ptr + step.offset, archive);
			}
		}
	}

	void ArchiveBinaryMapper::create(int classId, int offset) {
		auto* desc = Reflection::getDescriptor(classId);
		if (desc) {

			if (!(desc->flags & ClassDescriptor::NO_SERIALIZE)) {
				if (desc->isType<std::string>()) {
					Step step;
					step.plain = false;
					step.offset = offset;
					step.bytes = desc->size;
					step.writeCallback = [](const void* ptr, Archive& archive) {
						archive.writeStr(*(std::string*)ptr);
					};
					step.readCallback = [](void* ptr, Archive& archive) {
						archive.readStr(*(std::string*)ptr);
					};
					steps.push_back(step);
				}
				else if (desc->flags & ClassDescriptor::VECTOR) {
					Step step;
					step.plain = false;
					step.offset = offset;
					step.bytes = desc->size;
					int classId = desc->classId;
					step.writeCallback = [classId](const void* ptr, Archive& archive) {
						auto* desc = Reflection::getDescriptor(classId);
						if (desc) {
							int size = desc->vectorSize(ptr);
							archive.writeBin(size);
							ArchiveBinaryMapper *mapper = ArchiveBinaryMapper::getMapper(desc->elementType->classId);
							for (int i = 0; i < size; i++) {
								mapper->write(desc->vectorGet(ptr, i), archive);
							}
						}
					};
					step.readCallback = [classId](void* ptr, Archive& archive) {
						auto* desc = Reflection::getDescriptor(classId);
						if (desc) {
							int size = 0;
							archive.readBin(size);
							ArchiveBinaryMapper* mapper = ArchiveBinaryMapper::getMapper(desc->elementType->classId);
							for (int i = 0; i < size; i++) {
								desc->vectorInsert(ptr, i, nullptr);
								mapper->read(desc->vectorGet(ptr, i), archive);
							}
						}
					};
					steps.push_back(step);
				}
				else if (desc->flags & ClassDescriptor::REFERENCE) {
					if (desc->elementType->flags & ClassDescriptor::ASSET) {
						Step step;
						step.plain = false;
						step.offset = offset;
						step.bytes = desc->size;
						int classId = desc->elementType->classId;
						step.writeCallback = [](const void* ptr, Archive& archive) {
							auto file = env->assetManager->getFile(*(Ref<Asset>*)ptr);
							archive.writeStr(file);
						};
						step.readCallback = [classId](void* ptr, Archive& archive) {
							std::string file;
							archive.readStr(file);
							if (!file.empty()) {
								*(Ref<Asset>*)ptr = env->assetManager->get(classId, file);
							}
							else {
								*(Ref<Asset>*)ptr = nullptr;
							}
						};
						steps.push_back(step);
					}
				}
				else if (desc->properties.size() > 0) {
					for (auto& prop : desc->properties) {
						if (!(prop.flags & PropertyDescriptor::NO_SERIALIZE)) {
							create(prop.type->classId, offset + prop.offset);
						}
					}
				}
				else {

					if (!(
						desc->isType<int>() ||
						desc->isType<bool>() ||
						desc->isType<float>() ||
						desc->isType<double>() ||
						desc->isType<EntityId>() ||
						desc->isType<Guid>() ||
						desc->isType<Color>() ||
						desc->isType<glm::vec2>() ||
						desc->isType<glm::vec3>() ||
						desc->isType<glm::vec4>() ||
						desc->enumValues.size() > 0
						)) {
						return;
					}

					Step step;
					step.plain = true;
					step.offset = offset;
					step.bytes = desc->size;
					bool hasMerged = false;
					if (steps.size() > 0) {
						auto& back = steps.back();
						if (back.plain) {
							if (step.offset == back.offset + back.bytes) {
								back.bytes += step.bytes;
								hasMerged = true;
							}
						}
					}
					if (!hasMerged) {
						steps.push_back(step);
					}
				}
			}

		}
	}

	void StringArchive::writeStr(const std::string& str) {
		auto entry = idByStr.find(str);
		if (entry == idByStr.end()) {
			writeVarInt(0);
			Archive::writeStr(str);
			int id = idByStr.size() + 1;
			idByStr[str] = id;
			strById[id] = str;
		}
		else {
			writeVarInt(entry->second);
		}
	}

	void StringArchive::readStr(std::string& str) {
		int64_t id = 0;
		readVarInt(id);
		if (id == 0) {
			Archive::readStr(str);
			id = idByStr.size() + 1;
			idByStr[str] = id;
			strById[id] = str;
		}
		else {
			auto entry = strById.find(id);
			if (entry != strById.end()) {
				str = entry->second;
			}
			else {
				//error
			}
		}
	}

}
