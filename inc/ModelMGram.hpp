/* 
 * File:   ModelMGram.hpp
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
 * Created on October 28, 2015, 10:28 AM
 */

#ifndef MODELMGRAM_HPP
#define	MODELMGRAM_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "BaseMGram.hpp"

#include "TextPieceReader.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

#include "ByteMGramId.hpp"
#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

namespace uva {
    namespace smt {
        namespace tries {
            namespace m_grams {

                /**
                 * This class is used to represent the N-Gram that will be stored into the language model.
                 */
                template<typename WordIndexType, TModelLevel MAX_LEVEL_CAPACITY = M_GRAM_LEVEL_MAX>
                class T_Model_M_Gram : public T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY> {
                public:
                    //The type of the word id
                    typedef typename WordIndexType::TWordIdType TWordIdType;
                    //Define the corresponding M-gram id type
                    typedef m_gram_id::Byte_M_Gram_Id<TWordIdType> T_M_Gram_Id;
                    //Define the base class type
                    typedef T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY> BASE;

                    //Stores the m-gram probability, the log_10 probability of the N-Gram Must be a negative value
                    TLogProbBackOff m_prob;

                    //Stores the m-gram log_10 back-off weight (probability) of the N-gram can be 0 is the probability is not available
                    TLogProbBackOff m_back_off;

                    //Stores, if needed, the m-gram's context i.e. for "w1 w2 w3" -> "w1 w2"
                    TextPieceReader m_context;

                    /**
                     * The basic constructor, is to be used when the M-gram level
                     * is known beforehand. Allows to set the actual M-gram level
                     * to a concrete value.
                     * @param word_index the used word index
                     * @param actual_level the actual level of the m-gram that will be used should be <= MAX_LEVEL_CAPACITY
                     */
                    T_Model_M_Gram(WordIndexType & word_index, TModelLevel actual_level)
                    : T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY>(word_index, actual_level) {
                    }

                    /**
                     * Allows to start a new M-gram with the given level
                     * @param CURR_LEVEL the level of the M-gram we are starting
                     */
                    inline void start_new_m_gram() {
                        m_curr_index = BASE::m_actual_begin_word_idx;
                    }

                    /**
                     * Returns the reference to the next new token of the m-gram
                     * @return the reference to the next new token of the m-gram
                     */
                    inline TextPieceReader & get_next_new_token() {
                        if (DO_SANITY_CHECKS && (m_curr_index > BASE::m_actual_end_word_idx)) {
                            stringstream msg;
                            msg << "The next token does not exist, exceeded the maximum of " << SSTR(BASE::m_actual_level) << " elements!";
                            throw Exception(msg.str());
                        }
                        return BASE::m_tokens[m_curr_index++];
                    }

                    /**
                     * Allows to prepare the M-gram for being used for adding it to the trie
                     * This includes registering the one gram in the word index
                     */
                    inline void prepare_for_adding() {
                        //If we have a unigram then add it to the index otherwise get the word ids
                        if (BASE::m_actual_level == M_GRAM_LEVEL_1) {
                            if (BASE::m_word_index.is_word_registering_needed()) {
                                //Register the word if it is needed
                                BASE::m_word_ids[BASE::m_actual_begin_word_idx] = BASE::m_word_index.register_word(BASE::m_tokens[BASE::m_actual_begin_word_idx]);
                            }
                        } else {
                            //Store the word ids without the unknown word flags and pre-compute the m-gram hash values
                            TModelLevel curr_idx = BASE::m_actual_end_word_idx;
                            BASE::m_word_ids[curr_idx] = BASE::m_word_index.get_word_id(BASE::m_tokens[curr_idx]);
                            //The Unigram's hash value is equal to the word id
                            m_hash_values[curr_idx] = BASE::m_word_ids[curr_idx];

                            //Move on and compute the incremental hash
                            do {
                                //Decrement the index
                                curr_idx--;
                                //Get the next word id
                                BASE::m_word_ids[curr_idx] = BASE::m_word_index.get_word_id(BASE::m_tokens[curr_idx]);
                                //Compute the next hash value 
                                m_hash_values[curr_idx] = combine_hash(BASE::m_word_ids[curr_idx], m_hash_values[curr_idx + 1]);
                                //Stop iterating if the reached the beginning of the m-gram
                            } while (curr_idx != BASE::m_actual_begin_word_idx);
                        }
                    }

                    /**
                     * Allows to retrieve the hash value for the given m-gram
                     * @return the hash value for the given m-gram
                     */
                    inline uint64_t get_hash() const {
                        //The hash value is computed incrementally and backwards and therefore
                        //the full hash is stored under the index of the first n-gram's word
                        return m_hash_values[BASE::m_actual_begin_word_idx];
                    }

                    /**
                     * If the context is set to be all the m-gram tokens, then this method
                     * allows to exclude the last token from it. It also takes care of the
                     * space in between. Note that, this method must only be called if the
                     * context is initialized and it he m-gram level m is greater than one. 
                     */
                    inline void exclude_last_token_from_context() {
                        //The reduction factor for length is the length of the last m-gram's token plus
                        //one character which is the space symbol located between m-gram tokens.
                        const size_t reduction = (BASE::m_tokens[BASE::m_actual_end_word_idx].length() + 1);
                        m_context.set(m_context.get_begin_ptr(), m_context.length() - reduction);
                    }

                    /**
                     * Allows to create a new m-gram id for the given m-gram of the specific current level, and the back-off flag.
                     * For the argument reference to the id data pointer the following holds:
                     * a) If there was no memory allocated for the M-gram id then there will be allocated as much
                     * as needed to store the given id.
                     * b) If there was memory allocated then no re-allocation will be done, then it is assumed that enough memory was allocated
                     * @param CURR_LEVEL the current M-gram level to compute the id for
                     * @param IS_BACK_OFF true if the M-gram id is to be computed for the back-off M-gram of the given level, default is false.
                     * @param m_p_gram_id the reference to the M-gram id data pointer to be initialized with the M-gram id data, must be pre-allocated
                     */
                    template<TModelLevel CURR_LEVEL>
                    inline void create_m_gram_id(T_Gram_Id_Data_Ptr & m_p_gram_id) const {
                        //Perform the sanity check if needed
                        if (DO_SANITY_CHECKS && (CURR_LEVEL != BASE::m_actual_level)) {
                            stringstream msg;
                            msg << "The actual (" << SSTR(CURR_LEVEL) << ") and current ("
                                    << SSTR(BASE::m_actual_level) << ") level mismatch!";
                            throw Exception(msg.str());
                        }

                        BASE::template create_m_gram_id < BASE::m_actual_begin_word_idx, CURR_LEVEL - 1 > (m_p_gram_id);
                    }

                private:
                    //The data structure to store the N-gram hashes
                    uint64_t m_hash_values[MAX_LEVEL_CAPACITY] = {};

                    //Stores the m-gram idx for when adding m-gram tokens
                    TModelLevel m_curr_index;

                    /**
                     * Make this constructor private as it is not to be used.
                     */
                    T_Model_M_Gram(WordIndexType & word_index) : T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY>(word_index) {
                    }

                };

            }
        }
    }
}

#endif	/* MODELMGRAM_HPP */
