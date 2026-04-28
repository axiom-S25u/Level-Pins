#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/Popup.hpp>
using namespace geode::prelude;
// ery before u come screaming at me the other level pins does something else
static std::vector<int> getPinned() {
    std::vector<int> pins;
    int limit = Mod::get()->getSettingValue<int64_t>("limit");
    for (int i = 0; i < limit; i++) {
        int id = Mod::get()->getSavedValue<int>("pin_" + std::to_string(i), -1);
        if (id != -1) pins.push_back(id);
    }
    return pins;
} // another note, added the limit so some random dude wont come in my server/geode server help forum to complain about ANYTHING

static std::string getLevelName(int id) {
    return Mod::get()->getSavedValue<std::string>("pinname_" + std::to_string(id), "Level " + std::to_string(id));
}

static void addPin(int id, std::string const& name) {
    int limit = Mod::get()->getSettingValue<int64_t>("limit");
    std::vector<int> pins = getPinned();
    for (int p : pins) if (p == id) return;
    if ((int)pins.size() >= limit) {
        FLAlertLayer::create("Pins Full", "Unpin something first", "OK")->show();
        return;
    }
    int slot = (int)pins.size();
    Mod::get()->setSavedValue<int>("pin_" + std::to_string(slot), id);
    Mod::get()->setSavedValue<std::string>("pinname_" + std::to_string(id), name);
}

static void removePin(int id) {
    int limit = Mod::get()->getSettingValue<int64_t>("limit");
    std::vector<int> pins;
    for (int i = 0; i < limit; i++) {
        int val = Mod::get()->getSavedValue<int>("pin_" + std::to_string(i), -1);
        if (val != -1 && val != id) pins.push_back(val);
    }
    for (int i = 0; i < limit; i++)
        Mod::get()->setSavedValue<int>("pin_" + std::to_string(i), -1);
    for (int i = 0; i < (int)pins.size(); i++)
        Mod::get()->setSavedValue<int>("pin_" + std::to_string(i), pins[i]);
}

static bool isPinned(int id) {
    for (int p : getPinned()) if (p == id) return true;
    return false;
}

class $modify(PinMenuLayer, MenuLayer) {
    void addPinButton(CCMenu* menu, int lvlId, std::string const& lvlName, int idx) {
        CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
        spr->setScale(0.6f);

        CCLabelBMFont* lbl = CCLabelBMFont::create(lvlName.substr(0, 16).c_str(), "bigFont.fnt");
        lbl->setScale(0.35f);
        lbl->setPosition({spr->getContentSize().width / 2.0f, spr->getContentSize().height / 2.0f});
        spr->addChild(lbl);

        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(PinMenuLayer::onPinPlay)
        );
        btn->setTag(lvlId);
        btn->setID(fmt::format("pin-btn-{}"_spr, idx));
        menu->addChild(btn);
    }

    $override bool init() {
        if (!MenuLayer::init()) return false;

        std::vector<int> pins = getPinned();
        if (pins.empty()) return true;

        CCNode* rightMenu = this->getChildByID("right-side-menu");
        CCNode* leftMenu = this->getChildByID("side-menu");

        CCMenu* right = rightMenu ? typeinfo_cast<CCMenu*>(rightMenu) : nullptr;
        CCMenu* left = leftMenu ? typeinfo_cast<CCMenu*>(leftMenu) : nullptr;

        int rightCount = std::min(3, (int)pins.size());
        int leftCount = (int)pins.size() - rightCount;

        for (int i = 0; i < rightCount; i++) {
            if (right) addPinButton(right, pins[i], getLevelName(pins[i]), i);
        }
        if (right) right->updateLayout();

        for (int i = 0; i < leftCount; i++) {
            if (left) addPinButton(left, pins[rightCount + i], getLevelName(pins[rightCount + i]), rightCount + i);
        }
        if (left) left->updateLayout();

        return true;
    }

    void onPinPlay(CCObject* sender) {
        CCNode* btn = typeinfo_cast<CCNode*>(sender);
        if (!btn) return;
        int id = btn->getTag();

        GJSearchObject* obj = GJSearchObject::create(SearchType::Search, std::to_string(id));
        CCScene* browser = LevelBrowserLayer::scene(obj);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, browser));
    }
};

class $modify(PinLevelInfoLayer, LevelInfoLayer) {
    $override bool init(GJGameLevel* level, bool p1) {
        if (!LevelInfoLayer::init(level, p1)) return false;

        CCNode* leftMenu = this->getChildByID("left-side-menu");
        if (!leftMenu) return true;

        bool pinned = isPinned(level->m_levelID);

        CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
        spr->setScale(0.8f);

        CCLabelBMFont* pinLbl = CCLabelBMFont::create(pinned ? "Unpin" : "Pin", "bigFont.fnt");
        pinLbl->setScale(0.35f);
        pinLbl->setPosition({spr->getContentSize().width / 2.0f, spr->getContentSize().height / 2.0f});
        spr->addChild(pinLbl);

        CCMenuItemSpriteExtra* pinBtn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(PinLevelInfoLayer::onPinBtn)
        );
        pinBtn->setID("pin-btn"_spr);
        leftMenu->addChild(pinBtn);
        leftMenu->updateLayout();

        return true;
    }

    void onPinBtn(CCObject*) {
        GJGameLevel* lvl = this->m_level;
        int id = lvl->m_levelID;
        if (isPinned(id)) {
            removePin(id);
            FLAlertLayer::create("Unpinned", "Level unpinned.", "OK")->show();
        } else {
            addPin(id, lvl->m_levelName);
            FLAlertLayer::create("Pinned", "Level pinned.", "OK")->show();
        }

        CCNode* leftMenu = this->getChildByID("left-side-menu");
        if (!leftMenu) return;
        if (CCNode* old = leftMenu->getChildByID("pin-btn"_spr)) old->removeFromParent();

        bool nowPinned = isPinned(id);
        CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
        spr->setScale(0.8f);
        CCLabelBMFont* lbl = CCLabelBMFont::create(nowPinned ? "Unpin" : "Pin", "bigFont.fnt");
        lbl->setScale(0.35f);
        lbl->setPosition({spr->getContentSize().width / 2.0f, spr->getContentSize().height / 2.0f});
        spr->addChild(lbl);

        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(PinLevelInfoLayer::onPinBtn)
        );
        btn->setID("pin-btn"_spr);
        leftMenu->addChild(btn);
        leftMenu->updateLayout();
    }
};