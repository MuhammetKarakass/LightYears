#pragma once
#include <AbilitySystemComponent.h>
#include <imgui.h>
#include <map>

namespace vega
{
	struct EditorContext;
	struct EditorScriptCommand;
	class EditorGasDebugger
	{
	public:
		bool visible = false;
		void InstallSettings();
		void Build(EditorContext& context);
		void Clear();
		bool Execute(EditorContext&, const EditorScriptCommand&, std::string& detail);
	private:
		std::shared_ptr<ly::Actor> Target(EditorContext&) const;
		bool Section(int index, const char* name);
		void Refresh(sas::AbilitySystemComponent&);
		bool mOpen[5]{ true, true, true, true, true };
		ImGuiTextFilter mFilters[5];
		ImGuiTextFilter mEffectFilter;
		sas::AbilitySystemDebugSnapshot mSnapshot;
		ly::List<sas::GameplayEffectSpec> mDefinitions;
		std::weak_ptr<ly::Actor> mTarget;
		std::map<std::string, std::vector<std::pair<double, float>>> mHistory;
		struct TagNode
		{
			int count = 0;
			std::map<std::string, TagNode> children;
		};
		TagNode mTags;
		std::map<unsigned int, float> mPeriods;
		double mNextRefresh = 0;
		int mRefreshMs = 100;
		char mTag[256]{};
		std::string mStatus;
		bool mFocus = false;
	};
}
