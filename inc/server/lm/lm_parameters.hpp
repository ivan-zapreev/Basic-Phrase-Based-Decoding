/* 
 * File:   language_model_parameters.hpp
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
 * Created on February 4, 2016, 11:52 AM
 */

#ifndef LANGUAGE_MODEL_PARAMETERS_HPP
#define LANGUAGE_MODEL_PARAMETERS_HPP

#include "lm_consts.hpp"
#include "lm_configs.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    /**
                     * This structure is needed to store the language model parameters
                     */
                    typedef struct {
                        //The the connection string needed to connect to the model
                        string m_conn_string;
                    } lm_parameters;
                }
            }
        }
    }
}

#endif /* LANGUAGE_MODEL_PARAMETERS_HPP */

