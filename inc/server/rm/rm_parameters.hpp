/* 
 * File:   rm_parameters.hpp
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
 * Created on February 4, 2016, 11:53 AM
 */

#ifndef RM_PARAMETERS_HPP
#define RM_PARAMETERS_HPP

#include <string>
#include <ostream>
#include <utility>

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/text/string_utils.hpp"

#include "server/server_configs.hpp"
#include "server/common/feature_id_registry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::common;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {

                    /**
                     * This structure stores the reordering model parameters
                     */
                    struct rm_parameters_struct {
                        //Stores the configuration section name
                        static const string RM_CONFIG_SECTION_NAME;
                        //The RM connection string parameter name
                        static const string RM_CONN_STRING_PARAM_NAME;
                        //The feature weights parameter name
                        static const string RM_WEIGHTS_PARAM_NAME;
                        //The feature weight names
                        static const string RM_WEIGHT_NAMES[MAX_NUM_RM_FEATURES];
                        //The feature weight names
                        static size_t RM_WEIGHT_GLOBAL_IDS[MAX_NUM_RM_FEATURES];

                        //The the connection string needed to connect to the model
                        string m_conn_string;

                        //Stores the number of reordering model weights
                        size_t m_num_lambdas;

                        //Stores the reordering model weights
                        float m_lambdas[MAX_NUM_RM_FEATURES];

                        /**
                         * Allows to get the features weights used in the corresponding model.
                         * @param registry the feature registry entity
                         */
                        inline void get_weight_names(feature_id_registry & registry) {
                            //Declare the feature source 
                            const string source = __FILENAME__;
                            //Add the feature weight names and increment the weight count
                            for (size_t idx = 0; idx < m_num_lambdas; ++idx) {
                                //Add the feature global id to its name mapping
                                registry.add_feature(source, RM_WEIGHT_NAMES[idx], RM_WEIGHT_GLOBAL_IDS[idx]);
                            }
                        }

                        /**
                         * Allows to verify the parameters to be correct.
                         */
                        void finalize() {
                            //The number of lambdas must not exceed the maximum
                            ASSERT_CONDITION_THROW(((m_num_lambdas > MAX_NUM_RM_FEATURES) &&
                                    (m_num_lambdas != EIGHT_RM_FEATURES)),
                                    string("The number of ") + RM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" must be <= ") + to_string(MAX_NUM_RM_FEATURES));

                            //The number of features must be an even number
                            ASSERT_CONDITION_THROW(((m_num_lambdas % 2) == 1),
                                    string("The number of ") + RM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) + string(", must be even!"));

                            //The number of features must not exceed the maximum supported value
                            ASSERT_CONDITION_THROW((m_num_lambdas > EIGHT_RM_FEATURES),
                                    string("The number of ") + RM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" must be <= ") + to_string(EIGHT_RM_FEATURES));
                        }
                    };

                    //Typedef the structure
                    typedef rm_parameters_struct rm_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const rm_parameters & params) {
                        return stream << "RM parameters: { conn_string = " << params.m_conn_string
                                << ", " << rm_parameters::RM_WEIGHTS_PARAM_NAME << "(" << params.m_num_lambdas
                                << ") = " << array_to_string<float>(params.m_num_lambdas,
                                params.m_lambdas, RM_FEATURE_WEIGHTS_DELIMITER_STR)
                                << " }";
                    }
                }
            }
        }
    }
}

#endif /* RM_PARAMETERS_HPP */

