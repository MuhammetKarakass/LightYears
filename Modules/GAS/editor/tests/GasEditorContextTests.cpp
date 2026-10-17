#include <gas/GasEditorExtension.h>
#include <editor/EditorExtensions.h>
#include <framework/EngineContext.h>
#include <framework/content/ContentSchema.h>
#include <iostream>

int main()
{
	vega::EngineContext host;
	vega::EngineContext unrelated;
	vega::EngineContext::Scope unrelatedScope{ unrelated };
	vega::EditorExtensions extensions{ host };
	sas::RegisterEditorExtension(extensions);
	if (!host.ContentSchemas().Find("ability") || !host.ContentSchemas().Find("effect")) return 1;
	if (!unrelated.ContentSchemas().Types().empty()) return 2;
	if (extensions.tools.size() != 1 || extensions.panels.size() != 1 ||
		!extensions.commands.Find("gas-panel")) return 3;
	std::cout << "GAS editor registration uses its explicit host context\n";
	return 0;
}
