#pragma once

#include <DOMTypes.hpp>
#include "InternalTypes.hpp"


/// Detect the entries by merging lines (bottom -> up)
void DOMEntriesExtraction(DOMElement* document, ApplicationData* data, e_force_indent force_indent);
