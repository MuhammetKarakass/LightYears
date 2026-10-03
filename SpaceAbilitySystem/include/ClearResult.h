#pragma once

namespace sas
{
	// Outcome of a Clear request. Pending means the clear is still requested and unfinished because
	// retained cleanup remains or the request was deferred behind an open operation; it is retried
	// automatically at the next opportunity. Exceptions are reserved for failures of user callbacks.
	enum class [[nodiscard]] ClearResult { Completed, Pending };
}
