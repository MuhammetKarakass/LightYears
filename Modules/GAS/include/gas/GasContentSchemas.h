#pragma once
#include <framework/content/ContentSchema.h>

namespace ly { class EngineContext; }

namespace sas
{
	// Games can extend/replace these descriptors with their content loaders,
	// behavior validation, templates and vocabularies.
	ly::content::ContentField MakeContentField(const std::string& path, const char* example, bool curve = false);
	void RegisterContentSchemas(ly::EngineContext& context);
}
