//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "entity/Scene.h"
#include "engine/Input.h"
#include "engine/EntityInfo.h"
#include "engine/MeshComponent.h"
#include "engine/RigidBody.h"
#include "engine/AssetManager.h"
#include <imgui.h>

namespace tri {

    class EntitiesWindow : public EditorElement {
    public:
        EntityId active;
        EntityId hovered;
        EntityId lastClicked;
        bool lastClickedWasSelected;
        bool blockStateChange;
        bool control;
        bool shift;
        std::vector<EntityId> entityOrder;
        std::vector<EntityId> lastFrameEntityOrder;
        std::unordered_set<EntityId> visited;

        void startup() {
            name = "Entities";
            type = WINDOW;
            active = -1;
            hovered = -1;
            lastClicked = -1;
            lastClickedWasSelected = false;
            blockStateChange = false;
            control = false;
            shift = false;

            env->signals->sceneLoad.addCallback([&](Scene *scene){
                active = -1;
                hovered = -1;
                lastClicked = -1;
                lastClickedWasSelected = false;
                blockStateChange = false;
            });
        }

        void update() override {
            TRI_PROFILE("Entities");
            control = env->input->down(Input::KEY_LEFT_CONTROL) || env->input->down(Input::KEY_RIGHT_CONTROL);
            shift = env->input->down(Input::KEY_LEFT_SHIFT) || env->input->down(Input::KEY_RIGHT_SHIFT);

            updateHeader();

            ImGui::Separator();
            ImGui::BeginChild("");

            hovered = -1;
            entityOrder.clear();
            visited.clear();

            env->scene->view<>().each([&](EntityId id){
                if(env->scene->hasComponent<Transform>(id)){
                    Transform &t = env->scene->getComponent<Transform>(id);
                    if(t.parent == -1){
                        if(updateEntity(id)){
                            updateEntityChildren(id);
                            ImGui::TreePop();
                        }
                    }
                }else{
                    if(updateEntity(id)){
                        updateEntityChildren(id);
                        ImGui::TreePop();
                    }
                }
            });

            //entities with invalid parent id
            bool first = true;
            bool open = false;
            env->scene->view<>().each([&](EntityId id) {
                if (env->scene->hasComponent<Transform>(id)) {
                    Transform& t = env->scene->getComponent<Transform>(id);
                    if (t.parent != -1) {
                        if (!env->scene->hasEntity(t.parent)) {
                            if (first) {
                                open = ImGui::TreeNodeEx("", 0);
                                ImGui::SameLine();
                                if (ImGui::Selectable("<invalid parent>", false)) {}
                                first = false;
                            }
                            if (open) {
                                if (updateEntity(id)) {
                                    updateEntityChildren(id);
                                    ImGui::TreePop();
                                }
                            }
                        }
                    }
                }
            });
            if (!first) {
                ImGui::TreePop();
            }

            updateEntityDragging();
            updateShortcuts();
            updateMenu();

            lastFrameEntityOrder = entityOrder;

            ImGui::EndChild();
        }

        void updateEntityChildren(EntityId id){
            for(auto child : env->hierarchies->getChildren(id)){
                if(updateEntity(child)){
                    updateEntityChildren(child);
                    ImGui::TreePop();
                }
            }
        }

        bool updateEntity(EntityId id){
            if (visited.contains(id)) {
                return false;
            }
            visited.insert(id);
            entityOrder.push_back(id);
            ImGui::PushID(id);
            std::string label = "";
            if(env->scene->hasComponent<EntityInfo>(id)){
                label = env->scene->getComponent<EntityInfo>(id).name;
            }
            if(label.empty()){
                label = "<entity " + std::to_string(id) + ">";
            }

            bool selected = env->editor->selectionContext.isSelected(id);
            bool hasChildren = env->hierarchies->getChildren(id).size() > 0;

            ImGuiTreeNodeFlags flags = 0;
            if(!hasChildren){
                flags |= ImGuiTreeNodeFlags_Leaf;
            }
            bool open = ImGui::TreeNodeEx("", flags);
            ImGui::SameLine();
            if(ImGui::Selectable(label.c_str(), selected)){
                if(!blockStateChange){
                    clicked(id);
                }
            }

            if(ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax())){
                hovered = id;
            }

            updateEntityMenu(id);
            ImGui::PopID();
            return open;
        }

        void updateHeader(){
            if (ImGui::Button("Add Entity")) {
                ImGui::OpenPopup("add entity");
            }
            if (ImGui::BeginPopup("add entity")) {

                if (ImGui::MenuItem("Empty", nullptr, nullptr)) {
                    EntityId id = env->editor->entityOperations.addEntity();
                    env->scene->addComponents(id, EntityInfo());
                    env->editor->selectionContext.unselectAll();
                    env->editor->selectionContext.select(id);
                }

                if (ImGui::BeginMenu("Components")) {
                    for (auto& desc : env->reflection->getDescriptors()) {
                        if (desc && (desc->flags & Reflection::COMPONENT)) {
                            if (!desc->isType<EntityInfo>() && !desc->isType<NoHierarchyUpdate>()) {

                                if (desc->group.empty()) {
                                    if (ImGui::MenuItem(desc->name.c_str(), nullptr, nullptr)) {
                                        EntityId id = env->editor->entityOperations.addEntity();
                                        env->scene->addComponents(id, Transform(), EntityInfo(desc->name));
                                        env->scene->addComponent(desc->typeId, id);
                                        env->editor->selectionContext.unselectAll();
                                        env->editor->selectionContext.select(id);
                                    }
                                }
                                else {
                                    if (ImGui::BeginMenu(desc->group.c_str())) {
                                        if (ImGui::MenuItem(desc->name.c_str(), nullptr, nullptr)) {
                                            EntityId id = env->editor->entityOperations.addEntity();
                                            env->scene->addComponents(id, Transform(), EntityInfo(desc->name));
                                            env->scene->addComponent(desc->typeId, id);
                                            env->editor->selectionContext.unselectAll();
                                            env->editor->selectionContext.select(id);
                                        }
                                        ImGui::EndMenu();
                                    }
                                }


                            }
                        }
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Physics")) {
                    if (ImGui::MenuItem("Box")) {
                        RigidBody rb;
                        rb.type = RigidBody::DYNAMIC;
                        EntityId id = env->scene->addEntity(
                            EntityInfo("Box"),
                            Transform(),
                            Collider(),
                            rb,
                            MeshComponent(env->assets->get<Mesh>("models/cube.obj"))
                        );
                        env->editor->undo.entityAdded(id);
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(id);
                    }
                    if (ImGui::MenuItem("Ball")) {
                        RigidBody rb;
                        rb.type = RigidBody::DYNAMIC;
                        EntityId id = env->scene->addEntity(
                            EntityInfo("Ball"),
                            Transform(),
                            Collider(Collider::SPHERE),
                            rb,
                            MeshComponent(env->assets->get<Mesh>("models/sphere.obj"))
                        );
                        env->editor->undo.entityAdded(id);
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(id);
                    }
                    if (ImGui::MenuItem("Static Box")) {
                        RigidBody rb;
                        rb.type = RigidBody::STATIC;
                        EntityId id = env->scene->addEntity(
                            EntityInfo("Static Box"),
                            Transform(),
                            Collider(),
                            rb,
                            MeshComponent(env->assets->get<Mesh>("models/cube.obj"))
                        );
                        env->editor->undo.entityAdded(id);
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(id);
                    }
                    ImGui::EndMenu();
                }


                ImGui::EndPopup();
            }
        }

        void updateShortcuts(){
            if(control){
                if(env->input->pressed("D")){
                    env->editor->entityOperations.duplicateSelection();
                }
                if(env->input->pressed("C")){
                    if(env->editor->selectionContext.getSelected().size() == 1) {
                        for (auto &id : env->editor->selectionContext.getSelected()) {
                            env->editor->entityOperations.copyEntity(id);
                        }
                    }else{
                        env->console->warning("cannot copy multiple entities");
                    }
                }
                if(env->input->pressed("V")){
                    if(env->editor->entityOperations.wasEntityCopied()) {
                        EntityId copy = env->editor->entityOperations.pastEntity();
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(copy);
                    }
                }
            }
            if(env->input->pressed(Input::KEY_DELETE)){
                env->editor->entityOperations.removeSelection();
            }
        }

        void updateEntityMenu(EntityId id){
            if(ImGui::BeginPopupContextItem()){
                if(ImGui::MenuItem("Delete", "Delete")){
                    if(env->editor->selectionContext.isSelected(id)){
                        env->editor->entityOperations.removeSelection();
                    }else{
                        env->editor->entityOperations.removeEntity(id);
                        env->editor->selectionContext.unselect(id);
                    }
                }
                if(ImGui::MenuItem("Copy", "Ctrl+C")){
                    env->editor->entityOperations.copyEntity(id);
                }
                if(ImGui::MenuItem("Past", "Ctrl+V", false, env->editor->entityOperations.wasEntityCopied())){
                    EntityId copy = env->editor->entityOperations.pastEntity();
                    env->editor->selectionContext.unselectAll();
                    env->editor->selectionContext.select(copy);
                }
                if(ImGui::MenuItem("Duplicate", "Ctrl+D")){
                    if(env->editor->selectionContext.isSelected(id)){
                        env->editor->entityOperations.duplicateSelection();
                    }else{
                        EntityId copy = env->editor->entityOperations.duplicateEntity(id);
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(copy);
                    }
                }
                if(ImGui::MenuItem("Parent", nullptr, false, env->editor->selectionContext.getSelected().size() > 0)){
                    env->editor->undo.beginAction();
                    for (auto &id2 : env->editor->selectionContext.getSelected()) {
                        if(id != id2){
                            Transform& t = env->scene->getOrAddComponent<Transform>(id2);
                            Transform preEdit = t;
                            t.parent = id;
                            if(env->scene->hasComponent<Transform>(id)){
                                Transform &t2 = env->scene->getComponent<Transform>(id);
                                t.decompose(glm::inverse(t2.getMatrix()) * t.getMatrix());
                            }
                            env->editor->undo.componentChanged(env->reflection->getTypeId<Transform>(), id2, &preEdit);
                        }
                    }
                    env->editor->undo.endAction();
                }
                if(env->scene->hasComponent<Transform>(id)){
                    Transform &t = env->scene->getComponent<Transform>(id);
                    if(t.parent != -1){
                        if(ImGui::MenuItem("Unparent")){
                            Transform preEdit = t;
                            t.parent = -1;
                            t.decompose(t.getMatrix());
                            env->editor->undo.componentChanged(env->reflection->getTypeId<Transform>(), id, &preEdit);
                        }
                    }
                }
                if(ImGui::MenuItem("Save")){
                    env->editor->gui.fileGui.openBrowseWindow("Save", "Save Entity",
                                                              env->reflection->getTypeId<Prefab>(), [id](const std::string &file){
                        Prefab prefab;
                        prefab.copyEntity(id, env->scene);
                        prefab.save(file);
                    });
                }
                if(env->hierarchies->getChildren(id).size() > 0) {
                    if (ImGui::MenuItem("Recenter")) {
                        reTransform(id, true, false, false);
                    }
                    if (ImGui::MenuItem("Rescale")) {
                        reTransform(id, false, true, false);
                    }
                    if (ImGui::MenuItem("Rerotate")) {
                        reTransform(id, false, false, true);
                    }
                }

                ImGui::EndPopup();
            }
        }

        void updateMenu(){
            if (ImGui::BeginPopupContextWindow("entity", ImGuiMouseButton_Right, false)) {
                if (ImGui::MenuItem("Past", "Ctrl+V", false, env->editor->entityOperations.wasEntityCopied())) {
                    EntityId copy = env->editor->entityOperations.pastEntity();
                    env->editor->selectionContext.unselectAll();
                    env->editor->selectionContext.select(copy);
                }
                if (ImGui::MenuItem("Load")) {
                    env->editor->gui.fileGui.openBrowseWindow("Load", "Load Entity",
                                                              env->reflection->getTypeId<Prefab>(), [](const std::string &file){
                        Prefab prefab;
                        prefab.load(file);
                        EntityId id = prefab.createEntity(env->scene);
                        env->editor->undo.entityAdded(id);
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(id);
                    });
                }
                ImGui::EndPopup();
            }
        }

        void updateEntityDragging(){
            if(active != -1 && hovered != -1){
                if(hovered != active){

                    if(env->hierarchies->haveSameParent(active, hovered)) {
                        auto *pool = env->scene->getEntityPool();
                        while (true) {

                            int index = -1;
                            int targetIndex = -1;
                            for (int i = 0; i < entityOrder.size(); i++) {
                                EntityId id = entityOrder[i];
                                if (active == id) {
                                    index = i;
                                }
                                if (hovered == id) {
                                    targetIndex = i;
                                }
                            }
                            if (index == targetIndex) {
                                break;
                            }

                            if (index != -1 && targetIndex != -1) {
                                if (targetIndex > index) {
                                    int swapIndex = pool->getIndexById(entityOrder[index]);
                                    int swapTargetIndex = pool->getIndexById(entityOrder[index + 1]);
                                    pool->swap(swapIndex, swapTargetIndex);

                                    auto *tpool = env->scene->getComponentPool<Transform>();
                                    if(tpool->has(entityOrder[index]) && tpool->has(entityOrder[index+1])) {
                                        swapIndex = tpool->getIndexById(entityOrder[index]);
                                        swapTargetIndex = tpool->getIndexById(entityOrder[index + 1]);
                                        tpool->swap(swapIndex, swapTargetIndex);
                                    }

                                    std::swap(entityOrder[index], entityOrder[index + 1]);
                                    if (targetIndex == index + 1) {
                                        break;
                                    }
                                } else if (targetIndex < index) {
                                    int swapIndex = pool->getIndexById(entityOrder[index]);
                                    int swapTargetIndex = pool->getIndexById(entityOrder[index - 1]);
                                    pool->swap(swapIndex, swapTargetIndex);

                                    auto *tpool = env->scene->getComponentPool<Transform>();
                                    if(tpool->has(entityOrder[index]) && tpool->has(entityOrder[index - 1])){
                                        swapIndex = tpool->getIndexById(entityOrder[index]);
                                        swapTargetIndex = tpool->getIndexById(entityOrder[index - 1]);
                                        tpool->swap(swapIndex, swapTargetIndex);
                                    }

                                    std::swap(entityOrder[index], entityOrder[index - 1]);
                                    if (targetIndex == index - 1) {
                                        break;
                                    }
                                } else {
                                    break;
                                }
                            } else {
                                break;
                            }

                        }

                    }

                    blockStateChange = true;
                }
            }

            if(env->input->pressed(Input::MOUSE_BUTTON_LEFT)){
                active = hovered;
            }
            if(env->input->released(Input::MOUSE_BUTTON_LEFT)){
                active = -1;
                blockStateChange = false;
            }
        }

        void clicked(EntityId id){
            bool selected = env->editor->selectionContext.isSelected(id);
            if(!shift || lastClicked == -1) {
                if (!selected) {
                    if (!control) {
                        env->editor->selectionContext.unselectAll();
                    }
                    env->editor->selectionContext.select(id);
                    lastClickedWasSelected = true;
                } else {
                    if (!control && env->editor->selectionContext.getSelected().size() > 1) {
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(id);
                        lastClickedWasSelected = true;
                    } else {
                        env->editor->selectionContext.unselect(id);
                        lastClickedWasSelected = false;
                    }
                }
                lastClicked = id;
            }else {
                selectRange(id, lastClicked, lastClickedWasSelected);
                lastClicked = id;
            }
        }

        void selectRange(EntityId id1, EntityId id2, bool select){
            bool inRange = false;
            for (int i = 0; i < lastFrameEntityOrder.size(); i++) {
                EntityId id = lastFrameEntityOrder[i];
                bool change = false;
                if (id == id1 || id == id2) {
                    if(!inRange){
                        inRange = true;
                        change = true;
                    }
                }
                if(inRange){
                    if(select){
                        env->editor->selectionContext.select(id);
                    }else{
                        env->editor->selectionContext.unselect(id);
                    }
                }
                if (id == id1 || id == id2) {
                    if((inRange && !change) || (id == id1 && id == id2)){
                        inRange = false;
                    }
                }
            }
        }

        void reTransform(EntityId id, bool position, bool scale, bool rotation){
            Transform avg;
            int count = 0;
            for (auto child : env->hierarchies->getChildren(id)) {
                if (env->scene->hasComponent<Transform>(child)) {
                    Transform &t = env->scene->getComponent<Transform>(child);
                    Transform t2;
                    t2.decompose(t.getMatrix());
                    avg.position += t2.position;
                    avg.scale += t2.scale;
                    avg.rotation += t2.rotation;
                    count++;
                }
            }
            avg.position /= (float) count;
            avg.scale /= (float) count;
            avg.rotation /= (float) count;


            if (count > 0) {
                if (env->scene->hasComponent<Transform>(id)) {

                    env->editor->undo.beginAction();

                    Transform &t = env->scene->getComponent<Transform>(id);
                    if(!position){
                        avg.position = t.position;
                    }
                    if(!scale){
                        avg.scale = t.scale;
                    }else{
                        avg.scale = {1, 1, 1};
                    }
                    if(!rotation){
                        avg.rotation = t.rotation;
                    }else{
                        avg.rotation = {0, 0, 0};
                    }
                    glm::mat4 matrix = t.getMatrix() * glm::inverse(avg.calculateLocalMatrix());

                    Transform preEdit = t;
                    glm::mat4 parentMatrix = t.getMatrix() * glm::inverse(t.calculateLocalMatrix());
                    t.decompose(glm::inverse(parentMatrix) * glm::inverse(matrix) * t.getMatrix());
                    env->editor->undo.componentChanged(env->reflection->getTypeId<Transform>(), id, &preEdit);

                    for (auto child : env->hierarchies->getChildren(id)) {
                        if (env->scene->hasComponent<Transform>(child)) {
                            Transform &t = env->scene->getComponent<Transform>(child);
                            Transform preEdit = t;
                            glm::mat4 parentMatrix = t.getMatrix() * glm::inverse(t.calculateLocalMatrix());
                            t.decompose(glm::inverse(parentMatrix) * matrix * t.getMatrix());
                            env->editor->undo.componentChanged(env->reflection->getTypeId<Transform>(), child, &preEdit);
                        }
                    }

                    env->editor->undo.endAction();
                }
            }
        }

    };
    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement<EntitiesWindow>();
    }

}
