// Copyright 2026 danillissimo

#include "AttributesTraites/AnchorsTraits.h"

static sketch::FHeaderToolAttributeFilter GAnchorsFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FAnchors");
});
