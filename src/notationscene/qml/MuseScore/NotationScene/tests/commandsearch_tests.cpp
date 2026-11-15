/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gmock/gmock.h>
#include "../searchpopupmodel.h"
#include "actions/tests/mocks/actionsdispatchermock.h"
#include "context/tests/mocks/globalcontextmock.h"
using namespace testing;
using namespace muse;
using namespace muse::ui;
using namespace mu::notation;
namespace {
class CommandActions : public IUiActionsRegister {
public:
    std::vector<UiAction> entries;
    std::map<actions::ActionCode, UiActionState> states;
    void reg(const IUiActionsModulePtr&) override {}
    void unreg(const IUiActionsModulePtr&) override {}
    std::vector<UiAction> actionList() const override { return entries; }
    const UiAction& action(const actions::ActionCode& code) const override {
        for (const auto& entry : entries) if (entry.code == code) return entry;
        static UiAction empty; return empty;
    }
    const actions::ActionCode& parentActionCode(const actions::ActionCode& code) const override { return code; }
    async::Channel<UiActionList> actionsChanged() const override { return {}; }
    UiActionState actionState(const actions::ActionCode& code) const override {
        const auto it = states.find(code); return it == states.end() ? UiActionState{} : it->second;
    }
    async::Channel<actions::ActionCodeList> actionStateChanged() const override { return {}; }
    void add(std::string code, QString title, QString description = {}, bool enabled = true) {
        UiAction entry; entry.code = code;
        entry.title = TranslatableString::untranslatable(title);
        entry.description = TranslatableString::untranslatable(description);
        entries.push_back(entry); states[code] = { enabled, false };
    }
};
class CommandSearch : public Test {
protected:
    modularity::ContextPtr ctx = std::make_shared<modularity::Context>(756);
    std::shared_ptr<CommandActions> registry = std::make_shared<CommandActions>();
    std::shared_ptr<NiceMock<actions::ActionsDispatcherMock>> dispatcher = std::make_shared<NiceMock<actions::ActionsDispatcherMock>>();
    std::shared_ptr<NiceMock<mu::context::GlobalContextMock>> context = std::make_shared<NiceMock<mu::context::GlobalContextMock>>();
    actions::ActionCodeList registered;
    std::unique_ptr<SearchPopupModel> model;
    void SetUp() override {
        modularity::ioc(ctx)->registerExport<IUiActionsRegister>("command-test", registry);
        modularity::ioc(ctx)->registerExport<actions::IActionsDispatcher>("command-test", dispatcher);
        modularity::ioc(ctx)->registerExport<mu::context::IGlobalContext>("command-test", context);
        ON_CALL(*dispatcher, actionList()).WillByDefault([this] { return registered; });
        EXPECT_CALL(*dispatcher, dispatch(An<const actions::ActionCode&>())).Times(0);
        EXPECT_CALL(*context, currentNotation()).Times(0);
        model = std::make_unique<SearchPopupModel>(); model->setContext(ctx);
    }
    void TearDown() override { model.reset(); modularity::removeIoC(ctx); }
    void add(std::string code, QString title, QString description = {}, bool enabled = true) {
        registry->add(code,title,description,enabled); registered.push_back(code);
    }
    QString code(int row = 0) { return model->commandResults().at(row).toMap()["code"].toString(); }
};
TEST_F(CommandSearch, NameDescriptionAndMultipleWordsMatchWithoutDispatchOrSelectionChange) {
    add("transpose", "Transpose…", "Change the selected notes by an interval");
    add("staff-text", "Staff text", "Add text to this staff");
    model->search("> interval SELECTED"); ASSERT_EQ(model->commandResults().size(),1); EXPECT_EQ(code(),"transpose");
    model->search("> staff text"); ASSERT_EQ(model->commandResults().size(),1); EXPECT_EQ(code(),"staff-text");
    model->search("> no-such-command"); EXPECT_TRUE(model->commandResults().empty());
    model->search(""); EXPECT_TRUE(model->commandResults().empty());
}
TEST_F(CommandSearch, UnregisteredAndDuplicateEntriesAreExcludedAndResultCountIsBounded) {
    registry->add("not-registered","Do not show me");
    for(int i=0;i<40;++i) add("action-"+std::to_string(i),QString("Action %1").arg(i));
    registry->entries.push_back(registry->entries.back());
    model->search("> "); ASSERT_EQ(model->commandResults().size(),8);
    QSet<QString> codes; for(const auto& row:model->commandResults()) codes.insert(row.toMap()["code"].toString());
    EXPECT_EQ(codes.size(),8); EXPECT_FALSE(codes.contains("not-registered"));
}
TEST_F(CommandSearch, DisabledActionsAreVisibleButCannotExecute) {
    add("transpose","Transpose",{},false);
    model->search("> transpose"); ASSERT_EQ(model->commandResults().size(),1);
    EXPECT_FALSE(model->commandResults().front().toMap()["enabled"].toBool());
    EXPECT_FALSE(model->runCommand("transpose"));
}
TEST_F(CommandSearch, ExecutionRechecksStateAndRegistrationAndRejectsUnlistedCodes) {
    add("transpose","Transpose"); model->search("> transpose");
    EXPECT_FALSE(model->runCommand("delete"));
    registry->states["transpose"].enabled=false; EXPECT_FALSE(model->runCommand("transpose"));
    registry->states["transpose"].enabled=true; registered.clear(); EXPECT_FALSE(model->runCommand("transpose"));
    registered.push_back("transpose");
    EXPECT_CALL(*dispatcher,dispatch(actions::ActionCode("transpose"))).Times(1);
    EXPECT_TRUE(model->runCommand("transpose"));
}
TEST_F(CommandSearch, PrefixAndEnabledOrderingAreStableAcrossRegistryReordering) {
    add("last","Add tempo"); add("prefix-disabled","Tempo",{},false); add("prefix","Tempo marking");
    model->search("> tempo"); ASSERT_EQ(model->commandResults().size(),3); EXPECT_EQ(code(),"prefix");
    const auto original=model->commandResults(); std::reverse(registry->entries.begin(),registry->entries.end());
    model->search("> tempo"); EXPECT_EQ(model->commandResults(),original);
}
TEST_F(CommandSearch, ClosingTheWindowDuringDispatchDoesNotUseTheDestroyedModel) {
    add("close", "Close score"); model->search("> close");
    EXPECT_CALL(*dispatcher,dispatch(actions::ActionCode("close"))).WillOnce([this](const auto&) { model.reset(); });
    EXPECT_TRUE(model->runCommand("close"));
    EXPECT_EQ(model, nullptr);
}
}
