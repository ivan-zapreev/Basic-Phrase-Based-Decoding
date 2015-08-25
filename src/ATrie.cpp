/* 
 * File:   ATrie.cpp
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
 * Created on August 25, 2015, 29:27 PM
 */
#include "ATrie.hpp"

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            TLogProbBackOff ATrie<N>::getBackOffWeight(const TModelLevel level) {
                //Get the word hash for the en word of the back-off N-Gram
                const TShortId & wordId = getBackOffNGramEndWordHash();
                const TModelLevel boCtxLen = level - 1;
                //Set the initial back-off weight value to undefined!
                TLogProbBackOff back_off = ZERO_LOG_PROB_WEIGHT;

                LOG_DEBUG1 << "Computing back-off for an " << level
                        << "-gram the context length is " << boCtxLen << END_LOG;

                if (boCtxLen > 0) {
                    //Attempt to retrieve back-off weights
                    TLongId ctxId = UNDEFINED_WORD_ID;
                    try {
                        //Compute the context hash
                        ctxId = getQueryContextId(boCtxLen, true);

                        LOG_DEBUG3 << "Got query context id: " << ctxId << END_LOG;

                        //The context length plus one is M value of the M-Gram
                        const TProbBackOffEntryPair& entry = get_M_GramDataRef(level, wordId, ctxId);

                        //Obtained the stored back-off weight
                        back_off = entry.back_off;

                        LOG_DEBUG2 << "The " << level << "-Gram log_"
                                << LOG_PROB_WEIGHT_BASE << "( back-off ) for (word, context)=("
                                << wordId << ", " << ctxId << "), is: " << back_off << END_LOG;
                    } catch (out_of_range e) {
                        LOG_DEBUG << "Unable to find the " << (level)
                                << "-Gram entry for a (word, context)=("
                                << wordId << ", " << ctxId << "), need to back off!" << END_LOG;
                    }
                } else {
                    //We came to a zero context, which means we have an
                    //1-Gram to try to get the back-off weight from

                    //Attempt to retrieve back-off weights
                    try {
                        const TProbBackOffEntryPair & pbData = get_1_GramDataRef(wordId);

                        //Note that: If the stored back-off is UNDEFINED_LOG_PROB_WEIGHT then the back of is just zero
                        back_off = pbData.back_off;

                        LOG_DEBUG2 << "The 1-Gram log_" << LOG_PROB_WEIGHT_BASE
                                << "( back-off ) for word: " << wordId
                                << ", is: " << back_off << END_LOG;
                    } catch (out_of_range e) {
                        LOG_DEBUG << "Unable to find the 1-Gram entry for a word: " << wordId << ", nowhere to back-off!" << END_LOG;
                    }
                }

                LOG_DEBUG2 << "The chosen log back-off weight for context: " << level << " is: " << back_off << END_LOG;

                //Return the computed back-off weight it can be UNDEFINED_LOG_PROB_WEIGHT, which is zero - no penalty

                return back_off;
            }

            template<TModelLevel N>
            TLogProbBackOff ATrie<N>::computeLogProbability(const TModelLevel level) {
                //Compute the context length of the given M-Gram
                const TModelLevel ctxLen = level - 1;
                //Get the last word in the N-gram
                const TShortId & wordId = getNGramEndWordHash();

                LOG_DEBUG1 << "Computing probability for an " << level
                        << "-gram the context length is " << ctxLen << END_LOG;

                //Consider different variants based no the length of the context
                if (ctxLen > 0) {
                    //If we are looking for a M-Gram probability with M > 0, so not for a 1-Gram

                    //Attempt to retrieve probabilities
                    TLongId ctxId = UNDEFINED_WORD_ID;
                    try {
                        //Compute the context hash based on what is stored in _wordHashes and context length
                        ctxId = getQueryContextId(ctxLen, false);

                        LOG_DEBUG3 << "Got query context id: " << ctxId << END_LOG;

                        if (level == N) {
                            //If we are looking for a N-Gram probability
                            const TLogProbBackOff& prob = get_N_GramDataRef(wordId, ctxId);

                            LOG_DEBUG2 << "The " << N << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                                    << "( prob. ) for (word,context) = (" << wordId << ", "
                                    << ctxId << "), is: " << prob << END_LOG;

                            //Return the stored probability
                            return prob;
                        } else {
                            //If we are looking for a M-Gram probability with 1 < M < N

                            //The context length plus one is M value of the M-Gram
                            const TProbBackOffEntryPair& entry = get_M_GramDataRef(level, wordId, ctxId);

                            LOG_DEBUG2 << "The " << level
                                    << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                                    << "( prob. ) for (word,context) = ("
                                    << wordId << ", " << ctxId
                                    << "), is: " << entry.prob << END_LOG;

                            //Return the stored probability
                            return entry.prob;
                        }
                    } catch (out_of_range e) {
                        LOG_DEBUG << "Unable to find the " << (ctxLen + 1)
                                << "-Gram  prob for a (word,context) = ("
                                << wordId << ", " << ctxId
                                << "), need to back off!" << END_LOG;

                        const TLogProbBackOff back_off = getBackOffWeight(ctxLen);
                        const TLogProbBackOff probability = computeLogProbability(ctxLen);

                        LOG_DEBUG1 << "getBackOffWeight(" << ctxLen << ") = " << back_off
                                << ", computeLogProbability(" << ctxLen << ") = "
                                << probability << END_LOG;

                        LOG_DEBUG2 << "The " << ctxLen << " probability = " << back_off
                                << " + " << probability << " = " << (back_off + probability) << END_LOG;

                        //Do the back-off weight plus the lower level probability, we do a plus as we work with LOG probabilities
                        return back_off + probability;
                    }
                } else {
                    //If we are looking for a 1-Gram probability, no need to compute the context
                    try {
                        const TProbBackOffEntryPair & pbData = get_1_GramDataRef(wordId);

                        LOG_DEBUG2 << "The 1-Gram log_" << LOG_PROB_WEIGHT_BASE
                                << "( prob. ) for word: " << wordId
                                << ", is: " << pbData.prob << END_LOG;

                        //Return the stored probability
                        return pbData.prob;
                    } catch (out_of_range e) {
                        LOG_DEBUG << "Unable to find the 1-Gram entry for a word: "
                                << wordId << " returning:" << MINIMAL_LOG_PROB_WEIGHT
                                << " for log_" << LOG_PROB_WEIGHT_BASE << "( prob. )" << END_LOG;

                        //Return the default minimal probability for an unknown word

                        return MINIMAL_LOG_PROB_WEIGHT;
                    }
                }
            }

            template<TModelLevel N>
            void ATrie<N>::queryNGram(const vector<string> & ngram, SProbResult & result) {
                const TModelLevel level = ngram.size();
                //Check the number of elements in the N-Gram
                if ((1 <= level) && (level <= N)) {
                    //First transform the given M-gram into word hashes.
                    ATrie<N>::storeNGramHashes(ngram);

                    //Go on with a recursive procedure of computing the N-Gram probabilities
                    result.prob = computeLogProbability(level);

                    LOG_DEBUG << "The computed log_" << LOG_PROB_WEIGHT_BASE << " probability is: " << result.prob << END_LOG;
                } else {
                    stringstream msg;
                    msg << "An improper N-Gram size, got " << level << ", must be between [1, " << N << "]!";
                    throw Exception(msg.str());
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class ATrie<MAX_NGRAM_LEVEL>;
        }
    }
}
