#pragma once
#include <framework/content/ContentSchema.h>

namespace vega { class EngineContext; }

namespace sas
{
	// Games can extend/replace these descriptors with their content loaders,
	// behavior validation, templates and vocabularies.
	vega::content::ContentField MakeContentField(const std::string& path, const char* example, bool curve = false);
	void RegisterContentSchemas(vega::EngineContext& context);
}
