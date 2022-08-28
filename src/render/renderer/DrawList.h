//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "RenderBatch.h"

namespace tri {

	class DrawList {
	public:
		class Entry {
		public:
			Shader* shader;
			Mesh* mesh;
			glm::mat4 transform;
			Material* material;
			Color color;
			EntityId id;
		};

		void add(const Entry &entry);
		void sort();
		void submit(RenderBatchList &batches);
		void reset();

	private:
		std::vector<std::pair<uint64_t, Entry>> entries;
		std::vector<int> ordering;
	};

}
