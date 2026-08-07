#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attachment/AttachmentDefinition.h"

namespace ly
{
	class AttachmentLoadout
	{
	public:
		bool TryEquip(
			const AttachmentDefinition& definition,
			AttachmentHostKind hostKind,
			const List<GameplayTag>& hostCapabilities,
			size_t slotCapacity,
			std::string* failureReason = nullptr
		);
		bool Remove(const std::string& attachmentId, AttachmentHostKind hostKind);
		void Clear();

		const List<EquippedAttachment>& GetEquipped() const { return mEquipped; }
		uint64_t GetRevision() const { return mRevision; }
		sas::GameplayAttributeList MergeGrantedAttributes(
			AttachmentHostKind hostKind,
			const sas::GameplayAttributeList& sourceAttributes
		) const;
		sas::GameplayAttribute ApplyStaticModifiers(
			AttachmentHostKind hostKind,
			const sas::GameplayAttribute& attribute
		) const;
		sas::GameplayAttributeList ApplyConditionalModifiers(
			AttachmentHostKind hostKind,
			const sas::GameplayAttributeList& resolvedAttributes,
			const List<GameplayTag>& originalDamageTags
		) const;
		List<GameplayTag> ResolveDamageTags(
			AttachmentHostKind hostKind,
			const List<GameplayTag>& baseDamageTags
		) const;
		float ResolveGrantedAttributeValue(
			AttachmentHostKind hostKind,
			const GameplayTag& attributeId,
			float fallback = 0.f
		) const;

	private:
		bool HasCapacity(AttachmentHostKind hostKind, size_t slotCapacity) const;
		bool HasAllCapabilities(
			const AttachmentDefinition& definition,
			const List<GameplayTag>& hostCapabilities
		) const;
		bool IsConditionMet(
			const AttachmentCondition& condition,
			const sas::GameplayAttributeList& resolvedAttributes,
			const List<GameplayTag>& originalDamageTags
		) const;
		List<sas::AttributeModifier> CollectStaticModifiers(AttachmentHostKind hostKind) const;

		List<EquippedAttachment> mEquipped;
		uint64_t mRevision = 1;
	};
}
