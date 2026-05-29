#pragma once

#include "Ext4/Structures.hpp"
#include "Ext4/Journal.hpp"
#include "Ext4/Inode.hpp"
#include "Ext4/Flags.hpp"
#include "Ext4/Image.hpp"

/**
 * @brief Namespace principal do projeto, contendo as definições de estruturas, classes e funções relacionadas ao sistema de arquivos EXT4.
 * @author Pedro Itiro Nagao
 */
namespace Ext4 {
    using namespace Structures;
    using namespace Journal;
    using namespace Flags;
    using namespace Inode;
}