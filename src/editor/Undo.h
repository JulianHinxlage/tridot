//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_UNDO_H
#define TRIDOT_UNDO_H

#include <functional>

namespace tridot {

	class Undo {
	public:
		void addAction(const std::function<void()> &undo, const std::function<void()> &redo = nullptr);
		void undoAction();
		void redoAction();

		std::vector<std::function<void()>> undos;
		std::vector<std::function<void()>> redos;
		std::vector<std::function<void()>> actions;
		std::vector<std::function<void()>> reverts;
	};

}

#endif //TRIDOT_UNDO_H
