/* 
 * File:   tm_source_entry.hpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.#
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created on February 9, 2016, 5:34 PM
 */

#ifndef TM_SOURCE_ENTRY_HPP
#define TM_SOURCE_ENTRY_HPP

#include <string>
#include <unordered_map>

#include "server/tm/tm_configs.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/containers/fixed_size_hashmap.hpp"

#include "server/tm/models/tm_phrase_id.hpp"
#include "server/tm/models/tm_target_entry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::hashing;
using namespace uva::utils::containers;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace models {

                        //Define the map storing the source phrase ids and the number of translations per phrase
                        typedef unordered_map<phrase_uid, size_t> sizes_map;

                        /**
                         * This is the source entry data structure that contains two things
                         * The source phrase uid, which is the unique identifier of the
                         * source string and the map storing the target translations.
                         * Note that the source phrase is not stored, this is to reduce
                         * memory consumption and improve speed. Similar as we did for
                         * the g2dm tried implementation for the language model.
                         */
                        class tm_source_entry {
                        public:

                            /**
                             * The basic constructor
                             */
                            tm_source_entry()
                            : m_phrase_uid(UNDEFINED_PHRASE_ID), m_capacity(0), m_targets(NULL), m_next_idx(0) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_source_entry() {
                                if (m_targets != NULL) {
                                    delete[] m_targets;
                                    m_targets = NULL;
                                }
                            }

                            /**
                             * Allows to set the source phrase id
                             * @param phrase_uid the source phrase id
                             */
                            inline void set_source_uid(phrase_uid phrase_uid) {
                                m_phrase_uid = phrase_uid;
                            }

                            /**
                             * Should be called to start the source entry, i.e. initialize the memory
                             * @param capacity the number of translations for this entry
                             */
                            inline void begin(const size_t capacity) {
                                //Store the number of translation entries
                                m_capacity = capacity;
                                //Instantiate the translations array
                                m_targets = new tm_target_entry[m_capacity]();
                            }

                            /**
                             * Should be called to indicate that this source entry is finished, i.e. all the translations have been set.
                             */
                            inline void finalize() {
                                //Nothing to be done at the moment
                            }

                            /**
                             * Allows to add a new translation to the source entry for the given target phrase
                             * @param target the target phrase string 
                             * @return the newly allocated target entry
                             */
                            inline tm_target_entry & new_translation(const string & target) {
                                //Perform a sanity check
                                ASSERT_SANITY_THROW((m_next_idx >= m_capacity),
                                        string("Exceeding the source entry capacity: ") + to_string(m_capacity));
                                //Get the next free entry for the target phrase
                                tm_target_entry & entry = m_targets[m_next_idx++];
                                //Set the entry's target phrase and its id
                                entry.set_target(target);
                                //Return the entry
                                return entry;
                            }

                            /**
                             * The comparison operator, allows to compare source entries
                             * @param phrase_uid the unique identifier of the source entry to compare with
                             * @return true if the provided uid is equal to the uid of this entry, otherwise false 
                             */
                            inline bool operator==(const phrase_uid & phrase_uid) const {
                                return (m_phrase_uid == phrase_uid);
                            }

                        private:
                            //Stores the unique identifier of the given source
                            phrase_uid m_phrase_uid;
                            //Stores the number of translation entries
                            size_t m_capacity;
                            //Stores the target entries array pointer
                            tm_target_entry * m_targets;
                            //Stores the next index for the translation entry
                            size_t m_next_idx;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_SOURCE_ENTRY_HPP */

