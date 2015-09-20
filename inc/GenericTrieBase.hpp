/* 
 * File:   GenericTrieBase.hpp
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
 * Created on September 20, 2015, 5:21 PM
 */

#ifndef GENERICTRIEBASE_HPP
#define	GENERICTRIEBASE_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "MGrams.hpp"
#include "TextPieceReader.hpp"

#include "MGramQuery.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "WordIndexTrieBase.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::mgrams;
using namespace uva::utils::math::bits;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This structure is used to define the trivial probability/
             * back-off pari to be stored for M-grams with 1 <= M < N
             * @param prob stores the probability
             * @param back_off stores the back-off
             */
            typedef struct {
                TLogProbBackOff prob;
                TLogProbBackOff back_off;
            } TProbBackOffEntry;

            /**
             * This class defined the trie interface and functionality that is expected by the TrieDriver class
             */
            template<TModelLevel N, typename WordIndexType>
            class GenericTrieBase : public WordIndexTrieBase<N, WordIndexType> {
            public:
                typedef MGramQuery<N, WordIndexType> TMGramQuery;

                //The offset, relative to the M-gram level M for the mgram mapping array index
                const static TModelLevel MGRAM_IDX_OFFSET = 2;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_GRAM_LEVELS = N - MGRAM_IDX_OFFSET;

                //Will store the the number of M levels such that 1 < M <= N.
                const static TModelLevel NUM_M_N_GRAM_LEVELS = N - 1;

                //Compute the N-gram index in in the arrays for M and N grams
                static const TModelLevel N_GRAM_IDX_IN_M_N_ARR = N - MGRAM_IDX_OFFSET;

                // Stores the undefined index array value
                static const TShortId UNDEFINED_ARR_IDX = 0;

                // Stores the undefined index array value
                static const TShortId FIRST_VALID_CTX_ID = UNDEFINED_ARR_IDX + 1;
                
                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit GenericTrieBase(WordIndexType & word_index) :WordIndexTrieBase<N, WordIndexType> (word_index){
                }

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @param gram the 1-Gram data
                 */
                inline void add_1_gram(const T_M_Gram &gram) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param gram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                inline void add_m_gram(const T_M_Gram & gram) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it snot guaranteed that the parameter will be checked to be a N-Gram!
                 * @param gram the N-Gram data
                 */
                inline void add_n_gram(const T_M_Gram & gram) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * This function allows to retrieve the probability stored for the given M-gram level.
                 * If the value is found then it must be set to the prob parameter of the function.
                 * If the value is not found then the prob parameter of the function must not be changed.
                 * @param level the level of the M-gram we need to compute probability for.
                 * @param prob the probability variable that is to be set with the found probability weight
                 * @return true if the probability for the given M-gram level could be found, otherwise false.
                 */
                inline void get_prob_weight(TMGramQuery & query) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * This function allows to retrieve the back-off stored for the given M-gram level.
                 * If the value is found then it must be added to the prob parameter of the function.
                 * If the value is not found then the prob parameter of the function must not be changed.
                 * In that case the back-off weight is just zero.
                 * @param level the level of the M-gram we need to compute probability for.
                 * @param prob the probability variable that is to be increased with the found back-off weight
                 */
                inline void add_back_off_weight(TMGramQuery & query) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to check if the given sub-m-gram contains an unknown word
                 * @param level of the considered M-gram
                 * @return true if the unknown word is present, otherwise false
                 */
                template<bool is_back_off>
                inline bool is_bitmap_hash_cache(TMGramQuery & query) {
                    THROW_MUST_OVERRIDE();
                };
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, BasicWordIndex >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, CountingWordIndex>;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> >;
        }
    }
}


#endif	/* GENERICTRIEBASE_HPP */

