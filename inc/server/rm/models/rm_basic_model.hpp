/* 
 * File:   rm_basic_model.hpp
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
 * Created on February 8, 2016, 10:01 AM
 */

#ifndef RM_BASIC_MODEL_HPP
#define RM_BASIC_MODEL_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/rm/rm_consts.hpp"
#include "server/rm/models/rm_entry.hpp"
#include "server/rm/models/rm_query.hpp"

#include "common/utils/containers/fixed_size_hashmap.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::containers;

using namespace uva::smt::bpbd::server::common::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace models {

                        /**
                         * This class represents a basic reordering model implementation.
                         * The basic model is based on the fixed size hash map which is a
                         * self-implemented linear probing hash map also used in several
                         * tries. This basic model also does not store the phrases as is
                         * but rather the hash values thereof. So it is a hash based
                         * implementation which reduces memory but might occasionally
                         * result in collisions, the latter is not checked.
                         */
                        class rm_basic_model {
                        public:
                            //Store the UNK source phrase id 
                            const phrase_uid SOURCE_UNK_UID;
                            //Store the UNK target phrase id 
                            const phrase_uid TARGET_UNK_UID;
                            //Store the <s> phrase id 
                            const phrase_uid BEGIN_SENT_TAG_UID;
                            //Store the </s> phrase id 
                            const phrase_uid END_SENT_TAG_UID;

                            //Define the translations data map. It represents possible translations for some source phrase.
                            typedef fixed_size_hashmap<rm_entry, const phrase_uid &> rm_entry_map;

                            /**
                             * The basic class constructor
                             */
                            rm_basic_model()
                            : SOURCE_UNK_UID(get_phrase_uid<true>(rm::RM_UNK_SOURCE_PHRASE)),
                            TARGET_UNK_UID(get_phrase_uid<true>(rm::RM_UNK_TARGET_PHRASE)),
                            BEGIN_SENT_TAG_UID(get_phrase_uid<true>(lm::BEGIN_SENTENCE_TAG_STR)),
                            END_SENT_TAG_UID(get_phrase_uid<true>(lm::END_SENTENCE_TAG_STR)),
                            m_rm_data(NULL), m_unk_entry(NULL),
                            m_begin_entry(NULL), m_end_entry(NULL) {
                                LOG_DEBUG << "SOURCE_UNK_UID = " << SOURCE_UNK_UID << END_LOG;
                                LOG_DEBUG << "TARGET_UNK_UID = " << TARGET_UNK_UID << END_LOG;
                                LOG_DEBUG << "BEGIN_SENT_TAG_UID = " << BEGIN_SENT_TAG_UID << END_LOG;
                                LOG_DEBUG << "END_SENT_TAG_UID = " << END_SENT_TAG_UID << END_LOG;
                            }

                            /**
                             * The basic destructor
                             */
                            ~rm_basic_model() {
                                //Delete the model data if any
                                if (m_rm_data != NULL) {
                                    delete m_rm_data;
                                    m_rm_data = NULL;
                                }
                            }

                            /**
                             * This method allows to detect if the number of
                             * reordering entries is needed before the entries
                             * are being added.
                             * @return true as this model type uses filed-size hash maps 
                             */
                            inline bool is_num_entries_needed() const {
                                return true;
                            }

                            /**
                             * This method is needed to set the number of reordering entries in the model.
                             * @param num_entries the number of entries in the reordering model
                             */
                            inline void set_num_entries(size_t num_entries) {
                                LOG_DEBUG << "The number of source/translation pairs is: " << num_entries << END_LOG;

                                //Initialize the source entries map
                                m_rm_data = new rm_entry_map(__rm_basic_model::SOURCES_BUCKETS_FACTOR, num_entries);
                            }

                            /**
                             * Allows to add a new reordering entry to the model
                             * @param source_uid the source phrase uid
                             * @param target_uid the target phrase uid
                             * @return the reference to the newly allocated entry
                             */
                            inline rm_entry & add_entry(const phrase_uid & source_uid, const phrase_uid & target_uid) {
                                //Compute the id
                                const phrase_uid uid = combine_phrase_uids(source_uid, target_uid);

                                //Add the new entry end return its reference
                                rm_entry & entry = m_rm_data->add_new_element(uid);

                                //Set the identifier into the entry
                                entry.set_entry_uid(uid);

                                return entry;
                            }

                            /**
                             * This method must be called after the model is
                             * loaded in order to find the UNK/UNK phrase entry
                             */
                            inline void find_unk_entry() {
                                //Try to find the UNK/UNK entry
                                m_unk_entry = get_entry(SOURCE_UNK_UID, TARGET_UNK_UID);

                                LOG_DEBUG << "UNK->UNK reordering: " << array_to_string<prob_weight>(rm_entry::get_num_features(), m_unk_entry->get_weights()) << END_LOG;

                                //Assert on that the UNK/UNK entry is found!
                                ASSERT_CONDITION_THROW((m_unk_entry == NULL), string("Could not find the ") +
                                        RM_UNK_SOURCE_PHRASE + string("/") + RM_UNK_SOURCE_PHRASE +
                                        string(" entry in the reordering model!"));
                            }

                            /**
                             * Allows to get the reordering model entry for the given tag
                             */
                            inline void find_begin_end_entries() {
                                //Try to find the <s> ||| <s> entry
                                m_begin_entry = get_entry(BEGIN_SENT_TAG_UID, BEGIN_SENT_TAG_UID);

                                //Assert on that the <s> ||| <s> entry is found!
                                ASSERT_CONDITION_THROW(((m_begin_entry == NULL) || (m_begin_entry == m_unk_entry)),
                                        string("Could not find the ") + lm::BEGIN_SENTENCE_TAG_STR + string("/") +
                                        lm::BEGIN_SENTENCE_TAG_STR + string(" entry in the reordering model!"));

                                //Try to find the </s> ||| </s> entry
                                m_end_entry = get_entry(END_SENT_TAG_UID, END_SENT_TAG_UID);

                                //Assert on that the </s> ||| </s> entry is found!
                                ASSERT_CONDITION_THROW(((m_end_entry == NULL) || (m_end_entry == m_unk_entry)),
                                        string("Could not find the ") + lm::END_SENTENCE_TAG_STR + string("/") +
                                        lm::END_SENTENCE_TAG_STR + string(" entry in the reordering model!"));
                            }

                            /**
                             * Allows to get the sentence begin tag entry if found 
                             * @return the sentence begin tag entry if found or NULL
                             */
                            inline const rm_entry * get_begin_tag_entry() const {
                                return m_begin_entry;
                            }

                            /**
                             * Allows to get the sentence end tag entry if found 
                             * @return the sentence end tag entry if found or NULL
                             */
                            inline const rm_entry * get_end_tag_entry() const {
                                return m_end_entry;
                            }

                            /**
                             * Allows to detect whether the given entry is an entry for the unknown phrase pair
                             * @param entry the entry pointer to be checked for being from the UNK/UNK entry
                             * @return true if this entry is for the unknown phrase pair, otherwise false
                             */
                            inline bool is_unk_entry(const rm_entry * entry) const {
                                return (entry == m_unk_entry) || (*entry == *m_unk_entry);
                            }

                            /**
                             * Allows to get the reordering entry for the given source/target
                             * pair the latter is identified with a phrase id.
                             * In case the entry is not present we return the data for the UNK/UNK pair.
                             * @param uid the source/target phrase pair uid
                             * @return the reordering entry, always NOT NULL!
                             */
                            inline const rm_entry * get_entry(const phrase_uid uid) const {
                                //If this is not the unknown phrase id then go searching, otherwise return the unk
                                if (uid != UNKNOWN_PHRASE_ID) {
                                    //Get the entry for the given id
                                    rm_entry * entry = m_rm_data->get_element(uid, uid);

                                    //Check if the entry is not NULL if it is then return the UNK/UNK
                                    if (entry != NULL) {
                                        return entry;
                                    } else {
                                        return m_unk_entry;
                                    }
                                } else {
                                    return m_unk_entry;
                                }
                            }

                            /**
                             * Allows to get the reordering entry for the given source/target pair
                             * In case the entry is not present we return the data for the UNK/UNK pair.
                             * @param source_uid the source phrase uid
                             * @param target_uid the target phrase uid
                             * @return the reordering entry, always NOT NULL!
                             */
                            inline const rm_entry * get_entry(const phrase_uid & source_uid, const phrase_uid & target_uid) const {
                                if ((source_uid != UNKNOWN_PHRASE_ID) && (target_uid != UNKNOWN_PHRASE_ID)) {
                                    return get_entry(combine_phrase_uids(source_uid, target_uid));
                                } else {
                                    return m_unk_entry;
                                }
                            }

                            /**
                             * Allows to log the model type info
                             */
                            inline void log_model_type_info() {
                                LOG_USAGE << "Using the hash-based reordering model: " << __FILENAME__ << END_LOG;
                            }

                        private:
                            //Stores the translation model data
                            rm_entry_map * m_rm_data;
                            //Stores the pointer to the UNK/UNK entry if found
                            const rm_entry * m_unk_entry;
                            //Stores the pointer to the <s>/<s> entry if found
                            const rm_entry * m_begin_entry;
                            //Stores the pointer to the </s>/</s> entry if found
                            const rm_entry * m_end_entry;
                        };
                    }
                }
            }
        }
    }
}

#endif /* RM_BASIC_MODEL_HPP */

