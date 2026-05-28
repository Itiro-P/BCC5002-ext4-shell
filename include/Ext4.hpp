#pragma once

#include "Ext4/Structures.hpp"
#include "Ext4/Journal.hpp"
#include "Ext4/Image.hpp"
#include "Ext4/Flags.hpp"
#include "Ext4/Inode.hpp"

namespace Ext4 {
    using Structures::SuperBlock;
    using Structures::GroupDescriptor;
    using Structures::MmpStruct;
    using Structures::OrphanBlockTail;
    using Structures::OrphanBlockParser;
    using Structures::ExtentHeader;
    using Structures::ExtentIndex;
    using Structures::ExtentLeaf;
    using Structures::ExtentTail;
    using Structures::DirectoryFileType;
    using Structures::DirectoryEntry;
    using Structures::DxRootInfo;
    using Structures::DxEntry;
    using Structures::DxTail;
    using Structures::XattrHeader;
    using Structures::XattrEntry;
    using Structures::XattrNameIndex;

    using Journal::JournalBlockType;
    using Journal::JournalHeader;
    using Journal::JournalSuperBlock;
}