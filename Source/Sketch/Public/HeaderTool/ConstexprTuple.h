#pragma once
#include <type_traits>
#include <cstring>

#include "SketchNoUniqueAddress.h"

namespace sketch
{
	/** A simple tuple, but with FULL constexpr support */
	template <class ItemType = void, class... ItemTypes>
	struct TTuple
	{
		constexpr TTuple()
		{
			if constexpr (std::is_scalar_v<ItemType>)
			{
				Item = static_cast<ItemType>(0);
			}
			else if constexpr (std::is_default_constructible_v<ItemType>)
			{
				Item = {};
			}
			else
			{
				static_assert(std::is_trivially_constructible_v<ItemType>);
				std::memset(&Item, 0, sizeof(ItemType));
			}
		}

		constexpr TTuple(ItemType FirstItem, ItemTypes... Items)
			: Item(FirstItem)
			, Next(Items...)
		{}

		template <class ItemInitializerType, class... ItemInitializerTypes>
		constexpr TTuple(ItemInitializerType FirstItem, ItemInitializerTypes... Items)
			: Item(FirstItem)
			, Next(Items...)
		{
			static_assert(sizeof...(ItemInitializerTypes) <= sizeof...(ItemTypes), "Too much initializers");
		}

		template <class Type>
		constexpr const Type& Get() const { return CheckInvariants<Type>(), GetImpl<Type>(this); }

		template <class Type>
		constexpr Type& Get() { return CheckInvariants<Type>(), GetImpl<Type>(this); }

		template <int Index>
		constexpr auto& Get() { return CheckInvariants<Index>(), GetImpl<Index>(this); }

		template <int Index>
		constexpr const auto& Get() const { return CheckInvariants<Index>(), GetImpl<Index>(this); }

		template <auto Predicate>
			requires requires { Predicate.template operator()<ItemType>(); }
		constexpr const auto& Get() const { return GetImpl<Predicate>(this); }

		template <auto Predicate>
			requires requires { Predicate.template operator()<ItemType>(); }
		constexpr auto& Get() { return GetImpl<Predicate>(this); }

		template <auto Predicate>
			requires requires { Predicate.template operator()<ItemType>(); }
		constexpr const auto& TryGet() const { return TryGetImpl<Predicate>(this); }

		template <auto Predicate>
			requires requires { Predicate.template operator()<ItemType>(); }
		constexpr auto& TryGet() { return TryGetImpl<Predicate>(this); }

		template <class CallableType, class... ArgTypes>
		constexpr decltype(auto) ApplyAfter(CallableType&& Callable, ArgTypes&&... Args)
		{
			if constexpr (sizeof...(ItemTypes) == 0)
			{
				return Callable(std::forward<ArgTypes>(Args)..., Item);
			}
			else
			{
				return Next.template ApplyAfter<CallableType, ArgTypes...>(std::forward<CallableType>(Callable), std::forward<ArgTypes>(Args)..., Item);
			}
		}

		template <class CallableType, class... ArgTypes>
		constexpr decltype(auto) ApplyBefore(CallableType&& Callable, ArgTypes&&... Args)
		{
			TTuple<decltype(Args)...> PackedArgs{ std::forward<ArgTypes>(Args)... };
			return ApplyBeforeImpl<CallableType>(std::forward<CallableType>(Callable), PackedArgs);
		}

		template <bool bReverse = false, class CallableType>
		constexpr void ForEach(CallableType&& Callable)
		{
			if constexpr (!bReverse)
			{
				Callable(Item);
			}
			if constexpr (sizeof...(ItemTypes) > 0)
			{
				Next.template ForEach<bReverse>(std::forward<CallableType>(Callable));
			}
			if constexpr (bReverse)
			{
				Callable(Item);
			}
		}

		ItemType Item;
		[[SKETCH_NO_UNIQUE_ADDRESS]]
		TTuple<ItemTypes...> Next;

	private:
		template <class Type, class ThisType>
		static constexpr auto& GetImpl(ThisType* me)
		{
			if constexpr (std::is_same_v<Type, ItemType>)
			{
				return me->Item;
			}
			else
			{
				return me->Next.template Get<Type>();
			}
		}

		template <int Index, class ThisType>
		static constexpr auto& GetImpl(ThisType* me)
		{
			if constexpr (Index == 0)
			{
				return me->Item;
			}
			else
			{
				return me->Next.template Get<Index - 1>();
			}
		}

		template <auto Predicate, class ThisType>
			requires requires { Predicate.template operator()<ItemType>(); }
		static constexpr auto& GetImpl(ThisType* me)
		{
			if constexpr (Predicate.template operator()<ItemType>())
			{
				return me->Item;
			}
			else
			{
				static_assert(sizeof...(ItemTypes) > 0);
				return me->Next.template Get<Predicate>();
			}
		}

		template <auto Predicate, class ThisType>
			requires requires { Predicate.template operator()<ItemType>(); }
		static constexpr auto& TryGetImpl(ThisType* me)
		{
			if constexpr (Predicate.template operator()<ItemType>())
			{
				return me->Item;
			}
			else if constexpr (sizeof...(ItemTypes) > 0)
			{
				return me->Next.template TryGet<Predicate>();
			}
			else
			{
				return me->Next;
			}
		}

		template <class Type>
		static consteval void CheckInvariants()
		{
			constexpr int TailSize = []() consteval
			{
				if constexpr (sizeof...(ItemTypes))
					return (int(std::is_same_v<ItemTypes, Type>) + ...);
				return 0;
			}();
			constexpr int NumInstances = int(std::is_same_v<ItemType, Type>) + TailSize;
			static_assert(NumInstances > 0, "This type is not present in this tuple");
			static_assert(NumInstances < 2, "This type is used multiple times by this tuple");
		}

		template <int Index>
		static consteval void CheckInvariants()
		{
			static_assert(Index >= 0);
			static_assert(Index <= sizeof...(ItemTypes));
		}

		template <class CallableType, class... ArgTypes, class... PreviousItemsTypes>
		constexpr auto ApplyBeforeImpl(CallableType&& Callable, TTuple<ArgTypes...>& PackedArgs, PreviousItemsTypes&... PreviousItems)
		{
			if constexpr (sizeof...(ItemTypes) == 0)
			{
				return PackedArgs.ApplyAfter(Callable, PreviousItems..., Item);
			}
			else
			{
				return Next.template ApplyBeforeImpl<CallableType, ArgTypes...>(Callable, PackedArgs, PreviousItems..., Item);
			}
		}
	};

	template <>
	struct TTuple<void>
	{};

	template <class... ArgTypes>
	constexpr TTuple<std::decay_t<ArgTypes>...> MakeTuple(ArgTypes&&... Args) { return TTuple<std::decay_t<ArgTypes>...>(std::forward<ArgTypes>(Args)...); }

	template <class... ArgTypes>
	constexpr TTuple<ArgTypes&...> Tie(ArgTypes&... Args) { return TTuple<ArgTypes&...>(Args...); }
}
