//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "ClassUI.h"
#include "Editor.h"
#include "EditorCamera.h"
#include "engine/Color.h"
#include "engine/AssetManager.h"
#include "engine/Transform.h"
#include "engine/EntityEvent.h"
#include "engine/EntityInfo.h"
#include "engine/Camera.h"
#include "engine/MetaTypes.h"
#include "render/objects/FrameBuffer.h"
#include "window/Input.h"
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <glm/glm.hpp>
 
namespace tri {

	TRI_SYSTEM(ClassUI);

	void ClassUI::init() {
		env->systemManager->addSystem<Editor>();
		env->editor->classUI = this;
		enableReferenceEdit = true;

		addClassUI<int>([](const char* label, int *value, int *min, int *max, bool multiEdit) {
			if (min && max) {
				return ImGui::SliderInt(label, value, *min, *max);
			}
			else {
				return ImGui::DragInt(label, value);
			}
		});
		addClassUI<bool>([](const char* label, bool* value, bool* min, bool* max, bool multiEdit) {
			return ImGui::Checkbox(label, value);
		});
		addClassUI<float>([](const char* label, float* value, float* min, float* max, bool multiEdit) {
			if (min && max) {
				return ImGui::SliderFloat(label, value, *min, *max);
			}
			else {
				return ImGui::DragFloat(label, value, 0.1);
			}
		});
		addClassUI<double>([](const char* label, double* value, double* min, double* max, bool multiEdit) {
			return ImGui::InputDouble(label, value, 0.1);
		});
		addClassUI<EntityId>([](const char* label, EntityId* value, EntityId* min, EntityId* max, bool multiEdit) {
			bool change = false;

			std::string name = "None";
			if (*value != -1) {
				if (auto* info = env->world->getComponent<EntityInfo>(*value)) {
					name = info->name;
				}
			}

			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::InputText("", &name, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor();
			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem("select")) {
					env->editor->selectionContext->select(*value);
				}
				if (ImGui::MenuItem("clear")) {
					*value = -1;
				}
				ImGui::EndPopup();
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
				if (env->input->released(Input::MOUSE_BUTTON_LEFT)) {
					if (env->editor->dragEntityId != *value && env->editor->dragEntityId != -1) {
						*value = env->editor->dragEntityId;
						change = true;
					}
					env->editor->dragEntityId = -1;
				}
			}

			ImGui::SameLine();
			ImGui::Text("%s", label);
			return change;
		});

		addClassUI<glm::vec2>([](const char* label, glm::vec2* value, glm::vec2* min, glm::vec2* max, bool multiEdit) {
			if (min && max) {
				return ImGui::SliderFloat2(label, (float*)value, *(float*)min, *(float*)max);
			}
			else {
				return ImGui::DragFloat2(label, (float*)value, 0.1);
			}
		});
		addClassUI<glm::vec3>([](const char* label, glm::vec3* value, glm::vec3* min, glm::vec3* max, bool multiEdit) {
			if (min && max) {
				return ImGui::SliderFloat3(label, (float*)value, *(float*)min, *(float*)max);
			}
			else {
				return ImGui::DragFloat3(label, (float*)value, 0.1);
			}
		});
		addClassUI<glm::vec4>([](const char* label, glm::vec4* value, glm::vec4* min, glm::vec4* max, bool multiEdit) {
			if (min && max) {
				return ImGui::SliderFloat4(label, (float*)value, *(float*)min, *(float*)max);
			}
			else {
				return ImGui::DragFloat4(label, (float*)value, 0.1);
			}
		});


		addClassUI<Color>([](const char* label, Color* value, Color* min, Color* max, bool multiEdit) {
			glm::vec4 vec = value->vec();
			if (ImGui::ColorEdit4(label, (float*)&vec)) {
				*value = vec;
				return true;
			}
			return false;
		});
		addClassUI<std::string>([](const char* label, std::string* value, std::string* min, std::string* max, bool multiEdit) {
			return ImGui::InputText(label, value);
		});
		addClassUI<Transform>([](const char* label, Transform* value, Transform* min, Transform* max, bool multiEdit) {
			bool change = false;
			change |= ImGui::DragFloat3("position", (float*)&value->position, 0.1);
			change |= ImGui::DragFloat3("scale", (float*)&value->scale, 0.1);
			glm::vec3 degrees = glm::degrees(value->rotation);
			if (ImGui::DragFloat3("rotation", (float*)&degrees, 1)) {
				value->rotation = glm::radians(degrees);
				change = true;
			}
			return change;
		});
		addClassUI<Guid>([](const char* label, Guid* value, Guid* min, Guid* max, bool multiEdit) {
			bool change = false;
			ImGui::LabelText(label, value->toString().c_str());
			return change;
		});
		addClassUI<EntityEvent>([](const char* label, EntityEvent* value, EntityEvent* min, EntityEvent* max, bool multiEdit) {
			bool change = false;

			auto drag = [value]() {

				EntityEvent::Listener l;
				std::string func;
				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Select")) {
						for (auto* desc : Reflection::getDescriptors()) {
							if (desc && desc->flags & ClassDescriptor::COMPONENT) {
								EntityId id = env->world->getIdByComponent(value, desc->classId);
								if (id != -1) {
									void *comp = env->world->getComponent(id, desc->classId);
									int offset = (uint8_t*)value - (uint8_t*)comp;
									for (int propertyIndex = 0; propertyIndex < desc->properties.size(); propertyIndex++) {
										auto& pdesc = desc->properties[propertyIndex];
										if (pdesc.offset == offset) {
											env->editor->entityOperations->copyEvent(id, desc->classId, propertyIndex);
										}
									}
								}
							}
						}
					}
					if (ImGui::MenuItem("Connect", nullptr, nullptr, env->editor->entityOperations->hasCopiedFunction())) {
						env->editor->entityOperations->pastFunction(l.entityId, l.function.classId, l.function.functionIndex);
						value->addListener(l.entityId, l.function.classId, l.function.functionIndex);
					}
					ImGui::EndPopup();
				}

				//drag target
				if (env->editor->classUI->dragTargetFunc(l.function.classId, func, l.entityId)) {
					auto* desc = env->reflection->getDescriptor(l.function.classId);
					if (desc) {
						for (int i = 0; i < desc->functions.size(); i++) {
							if (desc->functions[i]->name == func) {
								l.function.functionIndex = i;
							}
						}
						value->addListener(l.entityId, l.function.classId, l.function.functionIndex);
					}
				}
			};

			if (ImGui::TreeNodeEx(label, 0)) {
				drag();
				change |= env->editor->classUI->draw(value->listeners);
				if (ImGui::Button("invoke")) {
					value->invoke();
				}
				ImGui::TreePop();
			}
			else {
				drag();
			}

			return change;
		});
		
		addClassUI<ComponentIdentifier>([](const char* label, ComponentIdentifier* value, ComponentIdentifier* min, ComponentIdentifier* max, bool multiEdit) {
			bool change = false;
			change |= env->editor->classUI->componentCombo(value->classId, "component");
			return change;
		});
		addClassUI<PropertyIdentifier>([](const char* label, PropertyIdentifier* value, PropertyIdentifier* min, PropertyIdentifier* max, bool multiEdit) {
			bool change = false;
			change |= env->editor->classUI->componentCombo(value->classId, "component");
			change |= env->editor->classUI->propertyCombo(value->classId, value->propertyIndex, "property");
			return change;
		});
		addClassUI<FunctionIdentifier>([](const char* label, FunctionIdentifier* value, FunctionIdentifier* min, FunctionIdentifier* max, bool multiEdit) {
			bool change = false;
			change |= env->editor->classUI->componentCombo(value->classId, "component");
			change |= env->editor->classUI->funcPropertyCombo(value->classId, value->functionIndex, "function");
			return change;
		});
		addClassUI<PropertyValueIdentifier>([](const char* label, PropertyValueIdentifier* value, PropertyValueIdentifier* min, PropertyValueIdentifier* max, bool multiEdit) {
			bool change = false;
			change |= env->editor->classUI->componentCombo(value->classId, "component");
			change |= env->editor->classUI->propertyCombo(value->classId, value->propertyIndex, "property");
			
			if (value->classId != -1) {
				if (auto* desc = Reflection::getDescriptor(value->classId)) {
					if (value->propertyIndex >= 0 && value->propertyIndex < desc->properties.size()) {
						auto& prop = desc->properties[value->propertyIndex];
						if (value->value.classId != prop.type->classId) {
							value->value.clear();
							value->value.set(prop.type->classId);
						}
						change |= env->editor->classUI->draw(prop.type->classId, value->value.get(), "value");
					}
				}
			}
			return change;
		});

		addClassUI<Ref<FrameBuffer>>([](const char* label, Ref<FrameBuffer>* value, Ref<FrameBuffer>* min, Ref<FrameBuffer>* max, bool multiEdit) {
			bool change = false;
			if (ImGui::TreeNode(label)) {
				if (value->get()) {
					auto v = value->get();
					ImGui::Text("size: %i, %i", (int)v->getSize().x, (int)v->getSize().y);

					std::vector<TextureAttachment> attachments;
					for (int i = 0; i < 16; i++) {
						attachments.push_back(TextureAttachment(COLOR + i));
					}
					attachments.push_back(DEPTH);
					attachments.push_back(STENCIL);

					for (TextureAttachment attachment : attachments) {
						auto texture = v->getAttachment(attachment);
						if (texture) {
							float aspect = 1;
							if (texture->getHeight() != 0) {
								aspect = (float)texture->getWidth() / (float)texture->getHeight();
							}
							ImGui::Image((void*)(size_t)texture->getId(), ImVec2(200 * aspect, 200), ImVec2(0, 1), ImVec2(1, 0));

							if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
								ImGui::SetDragDropPayload(env->reflection->getDescriptor<Texture>()->name.c_str(), &texture, sizeof(texture));
								ImGui::Text("Texture");
								ImGui::EndDragDropSource();
							}

							ImGui::SameLine();
							std::string name;
							if (auto* spec = v->getAttachmentSpec(attachment)) {
								name = spec->name;
							}
							if (name.empty()) {
								if (attachment == DEPTH) {
									name = "depth";
								}
								else if (attachment == STENCIL) {
									name = "stencil";
								}
								else if (attachment == COLOR) {
									name = "color";
								}
								else {
									name = "color " + std::to_string((int)attachment - COLOR);
								}
							}
							ImGui::Text("%s", name.c_str());
						}
					}
				}
				ImGui::TreePop();
			}
			return change;
		});
		addClassUI<Camera>([](const char* label, Camera* value, Camera* min, Camera* max, bool multiEdit) {
			bool change = false;
			env->editor->classUI->draw(env->reflection->getClassId<Camera>(), value, label, min, max, false, false, false);
			if (value->output) {
				if (ImGui::TreeNode("View")) {
					auto texture = value->output->getAttachment(TextureAttachment::COLOR);
					if (texture) {
						float aspect = 1;
						if (texture->getHeight() != 0) {
							aspect = (float)texture->getWidth() / (float)texture->getHeight();
						}
						ImGui::Image((void*)(size_t)texture->getId(), ImVec2(200 * aspect, 200), ImVec2(0, 1), ImVec2(1, 0));
			
						EntityId id = env->world->getIdByComponent(value);
						if (id != -1) {
							if (env->world->hasComponent<Transform>(id)) {
								Transform& transform = *env->world->getComponent<Transform>(id);
								if (ImGui::IsItemHovered()) {
									static EditorCamera editorCamera;
									env->editor->propertiesNoContext = true;
									env->editor->propertiesNoScroll = true;
									editorCamera.update(*value, transform);
								}
							}
						}
					}
					ImGui::TreePop();
				}
			}
			return change;
		});

	}

	void ClassUI::addClassUI(int classId, const std::function<bool(const char*, void*, void*, void*, bool multiValue)>& callback) {
		if (callbacks.size() <= classId) {
			callbacks.resize(classId + 1);
		}
		callbacks[classId] = callback;
	}

	bool ClassUI::draw(int classId, void* ptr, const char* label, void* min, void* max, bool multiValue, bool drawHidden, bool useClassUICallback) {
		auto *desc = Reflection::getDescriptor(classId);
		if (desc && (!(desc->flags & ClassDescriptor::HIDDEN) || drawHidden)) {
			if (!label) {
				label = desc->name.c_str();
			}

			//use a callback if one exists
			if (useClassUICallback && callbacks.size() > classId && callbacks[classId]) {
				return callbacks[classId](label, ptr, min, max, multiValue);
			}

			//enum
			if (desc->enumValues.size() > 0) {
				bool change = false;
				int &value = *(int*)ptr;
				const char* strValue = "";

				for (auto& enumValue : desc->enumValues) {
					if (value == enumValue.second) {
						strValue = enumValue.first.c_str();
						break;
					}
				}

				if (ImGui::BeginCombo(label, strValue)) {
					for (auto& enumValue : desc->enumValues) {
						if (ImGui::Selectable(enumValue.first.c_str(), value == enumValue.second)) {
							value = enumValue.second;
							change = true;
						}
					}
					ImGui::EndCombo();
				}
				return change;
			}

			//reference types
			if (desc->flags & ClassDescriptor::REFERENCE) {
				Ref<Asset>& asset = *(Ref<Asset>*)ptr;
				auto file = env->assetManager->getFile(asset);

				if (!asset) {
					file = "None";
				}

				if (desc->elementType) {
					bool change = false;
					int id = desc->elementType->classId;

					if (ImGui::BeginCombo(label, file.c_str())) {
						if (ImGui::Selectable("None", false)) {
							asset = nullptr;
							change = true;
						}
						for (auto& f : env->assetManager->getAssetList(id)) {
							if (ImGui::Selectable(f.c_str(), f == file)) {
								asset = env->assetManager->get(id, f);
								change = true;
							}
						}
						ImGui::EndCombo();
					}

					if (id != -1) {
						std::string path;
						if (env->editor->classUI->dragTarget(id, path)) {
							asset = env->assetManager->get(id, path);
							change = true;
						}
						env->editor->classUI->dragSource(id, file);
					}

					if (enableReferenceEdit && asset) {
						if (desc->elementType->properties.size() > 0) {
							if (ImGui::TreeNodeEx(desc->elementType->name.c_str())) {
								if (ImGui::BeginPopupContextItem()) {
									if (ImGui::MenuItem("Save")) {
										if (asset) {
											asset->save(env->assetManager->searchFile(file));
										}
									}
									ImGui::EndPopup();
								}
								change |= draw(id, asset.get());
								ImGui::TreePop();
							}
							else {
								if (ImGui::BeginPopupContextItem()) {
									if (ImGui::MenuItem("Save")) {
										if (asset) {
											asset->save(env->assetManager->searchFile(file));
										}
									}
									ImGui::EndPopup();
								}
							}
						}
					}
					return change;
				}
			}

			//vector
			if (desc->flags & ClassDescriptor::VECTOR) {
				int size = desc->vectorSize(ptr);
				bool change = false;

				for (int i = 0; i < size; i++) {
					ImGui::PushID(i);
					if (ImGui::Button("-")) {
						desc->vectorErase(ptr, i);
						change = true;
						ImGui::PopID();
						break;
					}
					ImGui::SameLine();
					if (ImGui::Button("+")) {
						desc->vectorInsert(ptr, i, nullptr);
						change = true;
						ImGui::PopID();
						break;
					}

					if (ImGui::BeginPopupContextItem()) {
						if (ImGui::MenuItem("Move Up", nullptr, nullptr, i > 0)) {
							void *v1 = desc->vectorGet(ptr, i - 1);
							void *v2 = desc->vectorGet(ptr, i);
							desc->elementType->swap(v1, v2);
						}
						if (ImGui::MenuItem("Move Down", nullptr, nullptr, i < size - 1)) {
							void* v1 = desc->vectorGet(ptr, i);
							void* v2 = desc->vectorGet(ptr, i + 1);
							desc->elementType->swap(v1, v2);
						}
						if (ImGui::MenuItem("Duplicate")) {
							void* v = desc->vectorGet(ptr, i);
							desc->vectorInsert(ptr, i, v);
						}
						ImGui::EndPopup();
					}

					ImGui::SameLine();
					std::string label = std::to_string(i);
					ImGui::TreePush();
					ImGui::TreePush();
					change |= draw(desc->elementType->classId, desc->vectorGet(ptr, i), label.c_str());
					ImGui::TreePop();
					ImGui::TreePop();
					ImGui::PopID();
				}
				if (ImGui::Button("+")) {
					change = true;
					desc->vectorInsert(ptr, size, nullptr);
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear")) {
					change = true;
					desc->vectorClear(ptr);
				}
				return change;
			}

			//no callback, so draw every property
			bool change = false;
			for (auto& prop : desc->properties) {
				if ((prop.flags & PropertyDescriptor::HIDDEN) && !drawHidden) {
					continue;
				}

				int id = prop.type->classId;

				bool needTree = true;
				//callback
				if (callbacks.size() > id && callbacks[id]) {
					needTree = false;
				}
				//enum
				if (prop.type->enumValues.size() > 0) {
					needTree = false;
				}
				//reference type
				if (prop.type->flags & ClassDescriptor::REFERENCE) {
					needTree = false;
				}

				if (!needTree) {
					change |= draw(id, (uint8_t*)ptr + prop.offset, prop.name.c_str(), prop.min, prop.max, multiValue, drawHidden);
				}
				else {
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
					if (prop.type->properties.size() == 0 && !(prop.type->flags & ClassDescriptor::VECTOR)) {
						flags |= ImGuiTreeNodeFlags_Leaf;
					}
					if (ImGui::TreeNodeEx(prop.name.c_str(), flags)) {
						change |= draw(prop.type->classId, (uint8_t*)ptr + prop.offset, prop.name.c_str(), prop.min, prop.max, multiValue, drawHidden);
						ImGui::TreePop();
					}
				}
			}


			for (int functionIndex = 0; functionIndex < desc->functions.size(); functionIndex++) {
				auto& func = desc->functions[functionIndex];
				if (ImGui::Button(func->name.c_str())) {
					func->invoke(ptr);
				}
				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Select")) {
						env->editor->entityOperations->copyFunction(env->world->getIdByComponent(ptr, classId), desc->classId, functionIndex);
					}
					if (ImGui::MenuItem("Connect", nullptr, nullptr, env->editor->entityOperations->hasCopiedEvent())) {
						EntityId id = -1;
						int propertyClassId = -1;
						int propertyIndex = -1;
						env->editor->entityOperations->pastEvent(id, propertyClassId, propertyIndex);

						auto *propertyDesc = Reflection::getDescriptor(propertyClassId);
						if (propertyDesc) {
							void *comp = env->world->getComponent(id, propertyClassId);
							if (comp) {
								if (propertyIndex >= 0 && propertyIndex < propertyDesc->properties.size()) {
									auto& pdesc = propertyDesc->properties[propertyIndex];
									if (pdesc.type == Reflection::getDescriptor<EntityEvent>()) {
										void* prop = (uint8_t*)comp + pdesc.offset;
										((EntityEvent*)prop)->addListener(env->world->getIdByComponent(ptr, classId), desc->classId, functionIndex);
									}
								}

							}
						}

					}
					ImGui::EndPopup();
				}
				dragSourceFunc(desc->classId, func->name, env->world->getIdByComponent(ptr, classId));
			}

			return change;
		}
		return false;
	}

	bool ClassUI::componentCombo(int &classId, const char* label){
		bool change = false;
		std::string preview = "";
		if (classId != -1) {
			if (auto* desc = Reflection::getDescriptor(classId)) {
				preview = desc->name;
			}
		}
		if (ImGui::BeginCombo(label, preview.c_str())) {
			for (auto* desc : Reflection::getDescriptors()) {
				if (desc && desc->flags & ClassDescriptor::COMPONENT && !(desc->flags & ClassDescriptor::HIDDEN)) {
					if (ImGui::Selectable(desc->name.c_str(), preview == desc->name)) {
						classId = desc->classId;
						change = true;
					}
				}
			}
			ImGui::EndCombo();
		}
		return change;
	}

	bool ClassUI::funcPropertyCombo(int classId, FunctionDescriptor** func, const char* label) {
		if (classId < 0) {
			return false;
		}
		bool change = false;
		if (auto* desc = Reflection::getDescriptor(classId)) {
			std::string preview = "";
			if (*func) {
				preview = (*func)->name;
			}
			if (ImGui::BeginCombo(label, preview.c_str())) {
				for (int i = 0; i < desc->functions.size(); i++) {
					auto& f = desc->functions[i];
					if (ImGui::Selectable(f->name.c_str(), preview == f->name)) {
						*func = f;
						change = true;
					}
				}
				ImGui::EndCombo();
			}
		}
		return change;
	}

	bool ClassUI::funcPropertyCombo(int classId, int& functionIndex, const char* label) {
		if (classId < 0) {
			return false;
		}
		bool change = false;
		if (auto* desc = Reflection::getDescriptor(classId)) {
			std::string preview = "";
			if (functionIndex >= 0 && functionIndex < desc->functions.size()) {
				preview = desc->functions[functionIndex]->name;
			}
			if (ImGui::BeginCombo(label, preview.c_str())) {
				for (int i = 0; i < desc->functions.size(); i++) {
					auto* func = desc->functions[i];
					if (ImGui::Selectable(func->name.c_str(), preview == func->name)) {
						functionIndex = i;
						change = true;
					}
				}
				ImGui::EndCombo();
			}
		}
		return change;
	}

	bool ClassUI::propertyCombo(int classId, int& propertyIndex, const char* label) {
		if (classId < 0) {
			return false;
		}
		bool change = false;
		if (auto* desc = Reflection::getDescriptor(classId)) {
			std::string preview = "";
			if (propertyIndex >= 0 && propertyIndex < desc->properties.size()) {
				preview = desc->properties[propertyIndex].name;
			}
			if (ImGui::BeginCombo(label, preview.c_str())) {
				for (int i = 0; i < desc->properties.size(); i++) {
					auto& prop = desc->properties[i];
					if (!(prop.flags & PropertyDescriptor::HIDDEN)) {
						if (ImGui::Selectable(prop.name.c_str(), preview == prop.name)) {
							propertyIndex = i;
							change = true;
						}
					}
				}
				ImGui::EndCombo();
			}
		}
		return change;
	}

	bool ClassUI::dragTarget(int classId, void* ptr) {
		auto *desc = Reflection::getDescriptor(classId);
		if (desc) {
			if (ImGui::BeginDragDropTarget()) {
				auto *payload = ImGui::AcceptDragDropPayload(desc->name.c_str());
				if (payload) {
					desc->copy(payload->Data, ptr);
					ImGui::EndDragDropTarget();
					return true;
				}
			}
		}
		return false;
	}

	bool ClassUI::dragSource(int classId, const void* ptr) {
		auto* desc = Reflection::getDescriptor(classId);
		if (desc) {
			if (ImGui::BeginDragDropSource()) {
				ImGui::SetDragDropPayload(desc->name.c_str(), ptr, desc->size);
				ImGui::EndDragDropSource();
				return true;
			}
		}
		return false;
	}

	bool ClassUI::dragTarget(int classId, std::string& str) {
		auto* desc = Reflection::getDescriptor(classId);
		if (desc) {
			if (ImGui::BeginDragDropTarget()) {
				auto* payload = ImGui::AcceptDragDropPayload(desc->name.c_str());
				if (payload) {
					str = std::string((char*)payload->Data, payload->DataSize);
					ImGui::EndDragDropTarget();
					return true;
				}
			}
		}
		return false;
	}

	bool ClassUI::dragSource(int classId, const std::string& str) {
		auto* desc = Reflection::getDescriptor(classId);
		if (desc) {
			if (ImGui::BeginDragDropSource()) {
				ImGui::SetDragDropPayload(desc->name.c_str(), str.data(), str.size());
				ImGui::Text("%s", str.c_str());
				ImGui::EndDragDropSource();
				return true;
			}
		}
		return false;
	}

	bool ClassUI::dragTargetFunc(int& classId, std::string& func, EntityId& entityId) {
		if (ImGui::BeginDragDropTarget()) {
			auto* payload = ImGui::AcceptDragDropPayload("event");
			if (payload) {
				std::string data = std::string((char*)payload->Data, payload->DataSize);
				auto parts = StrUtil::split(data, ";");

				if (parts.size() >= 3) {
					try {
						func = parts[0];
						classId = std::stoi(parts[1]);
						entityId = std::stoi(parts[2]);
					}
					catch (...) {}
				}

				ImGui::EndDragDropTarget();
				return true;
			}
		}
		return false;
	}

	bool ClassUI::dragSourceFunc(int classId, const std::string& func, EntityId entityId) {
		if (ImGui::BeginDragDropSource()) {
			std::string data = func + ";" + std::to_string(classId) + ";" + std::to_string(entityId);
			ImGui::SetDragDropPayload("event", data.data(), data.size());
			ImGui::Text("%s", func.c_str());
			ImGui::EndDragDropSource();
			return true;
		}
	}

}