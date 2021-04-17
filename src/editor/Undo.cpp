//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Undo.h"

namespace tridot {

	void Undo::addAction(const std::function<void()>& undo, const std::function<void()>& redo){
		undos.push_back(undo);
		actions.push_back(redo);
		redos.clear();
		reverts.clear();
	}

	void Undo::undoAction(){
		if (undos.size() > 0) {
			if (undos.back()) {
				undos.back()();
			}
			reverts.push_back(undos.back());
			undos.pop_back();
			redos.push_back(actions.back());
			actions.pop_back();
		}
	}

	void Undo::redoAction(){
		if (redos.size() > 0) {
			if (redos.back()) {
				redos.back()();
			}
			undos.push_back(reverts.back());
			reverts.pop_back();
			actions.push_back(redos.back());
			redos.pop_back();
		}
	}

}
