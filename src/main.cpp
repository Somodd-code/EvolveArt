#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "EvolvePopup.hpp"

using namespace geode::prelude;

class $modify(EvolveEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Evolve", "goldFont.fnt", "GJ_button_04.png", 0.8f),
            this,
            menu_selector(EvolveEditorUI::onOpenEvolve)
        );

        auto menu = CCMenu::create();
        menu->addChild(btn);
        menu->setPosition({0, 0});
        menu->setID("evolveart-button-menu"_spr);

        // Floating in the top-left, independent of the rest of the editor's
        // button layout so it can't collide with other mods' buttons.
        btn->setPosition({45, CCDirector::sharedDirector()->getWinSize().height - 45});

        this->addChild(menu, 20);
        return true;
    }

    void onOpenEvolve(CCObject*) {
        EvolvePopup::create()->show();
    }
};
