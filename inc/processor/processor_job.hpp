/* 
 * File:   processor_job.hpp
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
 * Created on July 25, 2016, 11:41 AM
 */

#ifndef PROCESSOR_JOB_HPP
#define PROCESSOR_JOB_HPP

#include <ostream>
#include <fstream>

#include "common/utils/id_manager.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"

#include "common/messaging/msg_base.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/job_id.hpp"
#include "common/messaging/status_code.hpp"

#include "processor/messaging/proc_req_in.hpp"
#include "processor_parameters.hpp"

using namespace std;

using namespace uva::utils;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::processor::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {

                //Forward class declaration
                class processor_job;
                //Typedef the processor job pointer
                typedef processor_job * proc_job_ptr;

                /**
                 * Allows to log the processor job into an output stream
                 * @param stream the output stream
                 * @param job the job to log
                 * @return the reference to the same output stream send back for chaining
                 */
                ostream & operator<<(ostream & stream, const processor_job & job);

                /**
                 * This is the processor job class:
                 * Responsibilities:
                 *    - A base class for the pre and post processor jobs
                 */
                class processor_job {
                public:

                    //Define the function type for the function used to set the job result
                    typedef function<void(proc_job_ptr) > done_job_notifier;

                    /**
                     * The basic constructor
                     * @param config the language configuration, might be undefined.
                     * @param session_id the id of the session from which the translation request is received
                     * @param req the pointer to the processor request, not NULL
                     */
                    processor_job(const language_config & config,
                            const session_id_type session_id, proc_req_in *req)
                    : m_config(config), m_session_id(session_id),
                    m_job_id(req->get_job_id()), m_num_tasks(req->get_num_tasks()),
                    m_req_tasks(NULL), m_tasks_count(0), m_notify_job_done_func(NULL) {
                        //Allocate the required-size array for storing the processor requests
                        m_req_tasks = new proc_req_in_ptr[m_num_tasks]();
                        //Add the request
                        add_request(req);
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~processor_job() {
                        //Delete the requests, only the received ones, not NULL
                        for (size_t idx = 0; idx < m_num_tasks; ++idx) {
                            if (m_req_tasks[idx] != NULL) {
                                delete m_req_tasks[idx];
                            }
                        }
                        //Delete the array of tasks
                        delete[] m_req_tasks;
                    }

                    /**
                     * Allows to retrieve the session id
                     * @return the session id
                     */
                    inline session_id_type get_session_id() const {
                        return m_session_id;
                    }

                    /**
                     * Allows to retrieve the job id as given by the client.
                     * The job id given by the balancer is retrieved by another method.
                     * @return the job id
                     */
                    inline job_id_type get_job_id() const {
                        return m_job_id;
                    }

                    /**
                     * Allows to set the function that should be called when the job is done
                     * @param notify_job_done_func the job done notification function
                     *              to be called when the translation job is finished
                     */
                    inline void set_done_job_notifier(done_job_notifier notify_job_done_func) {
                        m_notify_job_done_func = notify_job_done_func;
                    }

                    /**
                     * Performs the processor job
                     */
                    virtual void execute() = 0;

                    /**
                     * Allows to wait until the job is finished, this
                     * includes the notification of the job pool.
                     */
                    virtual void synch_job_finished() = 0;

                    /**
                     * Allows to cancel the given processor job. Calling this method
                     * indicates that the job is canceled due to the client disconnect.
                     * This method is synchronized on requests.
                     */
                    virtual void cancel() = 0;

                    /**
                     * Allows to add a new pre-processor request to the job
                     * @param req the pre-processor request
                     */
                    inline void add_request(proc_req_in * req) {
                        unique_guard guard(m_req_tasks_lock);

                        //Get the task index
                        uint64_t task_idx = req->get_task_idx();

                        //Assert sanity
                        ASSERT_SANITY_THROW((task_idx >= m_num_tasks),
                                string("Improper tasks index: ") + to_string(task_idx) +
                                string(", must be <= ") + to_string(m_num_tasks));

                        //Assert sanity
                        ASSERT_SANITY_THROW((m_req_tasks[task_idx] != NULL),
                                string("The task index ") + to_string(task_idx) +
                                string(" of the job request ") + to_string(m_job_id) +
                                string(" from session ") + to_string(m_session_id) +
                                string(" is already set!"));

                        //Store the task
                        m_req_tasks[task_idx] = req;

                        //Increment the number of tasks
                        ++m_tasks_count;
                    }

                    /**
                     * Allows to check if the job is complete and is ready for execution.
                     * This method is synchronized on requests.
                     * @return true if the job is complete and is ready for execution otherwise false
                     */
                    inline bool is_complete() {
                        unique_guard guard(m_req_tasks_lock);

                        return (m_tasks_count == m_num_tasks);
                    }

                protected:

                    /**
                     * Allows to dump the request text into the file with the given name.
                     * This method is NOT synchronized.
                     * @param file_name the file name to dump the file into
                     * @throws uva_exception if the text could not be saved
                     */
                    inline void store_text_to_file(const string & file_name) {
                        //Check if the requests complete
                        ASSERT_SANITY_THROW(!is_complete(),
                                string("The processor job is not complete, #tasks: ") +
                                to_string(m_num_tasks) + string(", #received: ") +
                                to_string(m_tasks_count));

                        //Open the output stream to the file
                        ofstream out_file(file_name);

                        //Check that the file is open
                        ASSERT_CONDITION_THROW(!out_file.is_open(),
                                string("Could not open: ") +
                                file_name + string(" for writing"));

                        //Iterate and output
                        for (size_t idx = 0; idx < m_num_tasks; ++idx) {
                            //Output the text to the file, do not add any new lines, put text as it is.
                            out_file << m_req_tasks[idx]->get_text();
                        }

                        //Close the file
                        out_file.close();

                        LOG_USAGE << "The text is stored into: " << file_name << END_LOG;
                    }

                    /**
                     * Allows to get the reference to the language config.
                     * It is possible that the configuration is not defined!
                     * This method is NOT synchronized.
                     * @return the reference to the language config
                     */
                    inline const language_config & get_lang_config() {
                        return m_config;
                    }

                    /**
                     * Allows to construct the text file name, differs depending
                     * on whether this is a source or target text.
                     * This method is NOT synchronized.
                     * @param is_source if true then it is a source text, if false then the target text
                     * @param is_input if true then it is an input text, if false then the output text
                     * @param job_uid_str [out] will be set to the job uid string
                     * @return the name of the text file, should be unique
                     */
                    template<bool is_source, bool is_input>
                    inline const string get_text_file_name(string & job_uid_str) {
                        //Set the job uid
                        job_uid_str = to_string(m_session_id) + "." + to_string(m_job_id);
                        //Compute the file name
                        return m_config.get_work_dir() + "/" + job_uid_str + "." +
                                (is_source ? "pre" : "post") + "." +
                                (is_source ? "in" : "out") + ".txt";
                    }

                    /**
                     * Allows to get the language string from the request.
                     * This method is NOT synchronized.
                     * @return the language string from the request.
                     */
                    inline const string get_language() {
                        return m_req_tasks[0]->get_language();
                    }
                    
                private:
                    //Stores the reference to the language config, might be undefined
                    const language_config & m_config;
                    //Stores the translation client session id
                    const session_id_type m_session_id;
                    //Stores the job id for an easy access
                    const job_id_type m_job_id;
                    //Stores the number of tasks associated with the job
                    const uint64_t m_num_tasks;
                    //Stores the lock for accessing the tasks array
                    mutex m_req_tasks_lock;
                    //Stores the array of pointers to the processor job requests
                    proc_req_in_ptr * m_req_tasks;
                    //Stores the current number of received requests
                    uint64_t m_tasks_count;

                    //The done job notifier
                    done_job_notifier m_notify_job_done_func;
                };
            }
        }
    }
}




#endif /* PROCESSOR_JOB_HPP */
