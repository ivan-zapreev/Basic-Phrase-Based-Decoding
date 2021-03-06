#!/usr/bin/env python
# coding=utf-8
# File:         post_process_nltk.py
# Originator:   Ondrej Dusek, Charles University in Prague
# Adapted by:   Dr. Ivan S. Zapreev
# Purpose: 
#   Post-process sentence in some languages such as:
#       'english', 'french', 'spanish', 'italian', and 'czech'
#   The process inclused de-tokenization and upper casing.
#   The true casing is done using NLTK POS parser or the truecaser of
#   Nils Reimers located at: https://github.com/nreimers/truecaser
#   in case the language model is available. The language model is to
#   be located in the same folder with the script and must have a name:
#      <language-name>.obj  for example english.obj
#   The original truecase project also contains the model generation script.
# The original scrit is obtained from:
#   https://github.com/ufal/mtmonkey
# And it was licensed as:
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# Visit my Linked-in profile:
#      <https://nl.linkedin.com/in/zapreevis>
# Visit my GitHub:
#      <https://github.com/ivan-zapreev>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Created on November 14, 2016, 11:07 AM
#

"""
A simple post-process for MT, it de-tokenizes the sentences and upper cases them
trying to use the NLTK POS tagging. This script supports languages such as:
    'english', 'french', 'spanish', 'italian', and 'czech'
The process inclused de-tokenization and upper casing.
The true casing is done using NLTK POS parser or the truecaser of
Nils Reimers located at: https://github.com/nreimers/truecaser
in case the language model is available. The language model is to
be located in the same folder with the script and must have a name:
    <language-name>.obj  for example english.obj
The original truecase project also contains the model generation script.

Command-line usage:

    ./post_process_nltk.py [-h] [-c] [-u] [-l LANG] [-m MODELS_DIR] [-e ENCODING] [-t TEMPL] [input-file output-file]
    
    -h = the help message
    -e = use the given encoding (default: UTF-8)
    -l = use rules for the given language: a full english name
    -c = capitalize sentences
    -u = upper case using the truecaser, requires the model
         will use the truecaser from:
             https://github.com/nreimers/truecaser
    -m = the folder to search the truecaser models in
    -t = the name of the template file to be generated, optional
      
    If no input and output files are given, the de-tokenizer will read
    STDIN and write to STDOUT.
"""

from __future__ import unicode_literals
from regex import Regex, UNICODE, IGNORECASE

import cPickle
import os
import os.path
import sys
import inspect
import logging
import getopt
import codecs

#Include the sub-folder into the system path for module import
cmd_subfolder = os.path.realpath(os.path.abspath(os.path.join(os.path.split(inspect.getfile( inspect.currentframe() ))[0],"truecase/truecaser")))
if cmd_subfolder not in sys.path:
    sys.path.insert(0, cmd_subfolder)

from Truecaser import getTrueCase

reload(sys)
DEFAULT_ENCODING = 'utf-8'
sys.setdefaultencoding(DEFAULT_ENCODING)

try:
    from nltk.tokenize import word_tokenize
    from nltk.tag import pos_tag
except ImportError:
    print '[!] You need to install nltk (http://nltk.org/index.html)'
    exit(1)

class PostProcessor(object):
    """\
    A simple post-processor class.
    """

    # Moses special characters de-escaping
    ESCAPES = [('&bar;', '|'),
               ('&lt;', '<'),
               ('&gt;', '>'),
               ('&bra;', '['),
               ('&ket;', ']'),
               ('&amp;', '&')]  # should go last to prevent double de-escaping

    # Contractions for different languages
    CONTRACTIONS = {'english': r'^\p{Alpha}+(\'(ll|ve|re|[dsm])|n\'t)$',
                    'french': r'^([cjtmnsdl]|qu)\'\p{Alpha}+$',
                    'spanish': r'^[dl]\'\p{Alpha}+$',
                    'italian': r'^\p{Alpha}*(l\'\p{Alpha}+|[cv]\'è)$',
                    'czech': r'^\p{Alpha}+[-–](mail|li)$', }

    def __init__(self, options={}):
        """\
        Constructor (pre-compile all needed regexes).
        """
        # process options
        self.moses_deescape = True if options.get('moses_deescape') else False
        self.language = options.get('language', 'english')
        self.is_capitalize = True if options.get('is_capitalize') else False
        self.is_true_case = True if options.get('is_true_case') else False
        
        #If the sentence is to be capitalized try loading the model
        if self.is_true_case:
            # get the models folder
            self.models_dir = options.get('models_dir', '.')
            # create the model file name
            model_file_name = self.models_dir + "/" + self.language + ".obj"
            # check that the model file exists
            if os.path.isfile(model_file_name):
                #Read the model file
                f = open(model_file_name, 'rb')
                self.uniDist = cPickle.load(f)
                self.backwardBiDist = cPickle.load(f)
                self.forwardBiDist = cPickle.load(f)
                self.trigramDist = cPickle.load(f)
                self.wordCasingLookup = cPickle.load(f)
                f.close()
            else:
                print "Unable to find the truecaser model for: ", self.language
                exit(1)
        
        # compile regexes
        self.__currency_or_init_punct = Regex(r'^[\p{Sc}\(\[\{\¿\¡]+$')
        self.__noprespace_punct = Regex(r'^[\,\.\?\!\:\;\\\%\}\]\)]+$')
        self.__cjk_chars = Regex(r'[\u1100-\u11FF\u2E80-\uA4CF\uA840-\uA87F'
                                 + r'\uAC00-\uD7AF\uF900-\uFAFF\uFE30-\uFE4F'
                                 + r'\uFF65-\uFFDC]')
        self.__final_punct = Regex(r'([\.!?])([\'\"\)\]\p{Pf}\%])*$')
        # language-specific regexes
        self.__fr_prespace_punct = Regex(r'^[\?\!\:\;\\\%]$')
        self.__contract = None
        if self.language in self.CONTRACTIONS:
            self.__contract = Regex(self.CONTRACTIONS[self.language],
                                    IGNORECASE)


    def truecase(self, sentence, lang):
        """\
        True case the sentence using the NLTP POS tagging.
        It shall capitalize NNP and NNPS entries
        """
        # tokenize the sentence for the given language
        tokens = word_tokenize(sentence, language=lang)

        # infer capitalization from the model
        tokens = getTrueCase(tokens, "as-is", self.wordCasingLookup,
                             self.uniDist, self.backwardBiDist,
                             self.forwardBiDist, self.trigramDist)
            
        #Return the result
        return ' '.join(tokens)

    
    def post_process(self, sentence):
        """\
        Detokenize the given sentence using current settings.
        """

        # strip leading/trailing space
        sentence = sentence.strip()
        
        # check if we need to perform capitalization
        if self.is_capitalize:
            #capitalize, if the sentence ends with a final punctuation
            if self.__final_punct.search(sentence):
                sentence = sentence[0].upper() + sentence[1:]
                
        # check if we need to do true casing
        if self.is_true_case:
            # call the sentence true casing
            sentence = self.truecase(sentence, self.language);
        
        # split sentence
        words = sentence.split(' ')
        # paste sentence back, omitting spaces where needed 
        sentence = ''
        pre_spc = ' '
        quote_count = {'\'': 0, '"': 0, '`': 0}
        for pos, word in enumerate(words):
            # remove spaces in between CJK chars
            if self.__cjk_chars.match(sentence[-1:]) and \
                    self.__cjk_chars.match(word[:1]):
                sentence += word
                pre_spc = ' '
            # no space after currency and initial punctuation
            elif self.__currency_or_init_punct.match(word):
                sentence += pre_spc + word
                pre_spc = ''
            # no space before commas etc. (exclude some punctuation for French)
            elif self.__noprespace_punct.match(word) and \
                    (self.language != 'french' or not
                     self.__fr_prespace_punct.match(word)):
                sentence += word
                pre_spc = ' '
            # contractions with comma or hyphen 
            elif word in "'-–" and pos > 0 and pos < len(words) - 1 \
                    and self.__contract is not None \
                    and self.__contract.match(''.join(words[pos - 1:pos + 2])):
                sentence += word
                pre_spc = ''
            # handle quoting
            elif word in '\'"„“”‚‘’`':
                # detect opening and closing quotes by counting 
                # the appropriate quote types
                quote_type = word
                if quote_type in '„“”':
                    quote_type = '"'
                elif quote_type in '‚‘’':
                    quote_type = '\''
                # exceptions for true Unicode quotes in Czech & German
                if self.language in ['czech', 'german'] and word in '„‚':
                    quote_count[quote_type] = 0
                elif self.language in ['czech', 'german'] and word in '“‘':
                    quote_count[quote_type] = 1
                # special case: possessives in English ("Jones'" etc.)                    
                if self.language == 'english' and sentence.endswith('s'):
                    sentence += word
                    pre_spc = ' '
                # really a quotation mark
                else:
                    # opening quote
                    if quote_count[quote_type] % 2 == 0:
                        sentence += pre_spc + word
                        pre_spc = ''
                    # closing quote
                    else:
                        sentence += word
                        pre_spc = ' '
                    quote_count[quote_type] += 1
            # keep spaces around normal words
            else:
                sentence += pre_spc + word
                pre_spc = ' '
        # de-escape chars that are special to Moses
        if self.moses_deescape:
            for char, repl in self.ESCAPES:
                sentence = sentence.replace(char, repl)
        # strip leading/trailing space
        sentence = sentence.strip()
        
        return sentence


def display_usage():
    """\
    Display program usage information.
    """
    print >> sys.stderr, __doc__


def open_handles(filenames, encoding):
    """\
    Open given files or STDIN/STDOUT in the given encoding.
    """
    if len(filenames) == 2:
        fh_out = codecs.open(filenames[1], 'w', encoding)
    else:
        fh_out = codecs.getwriter(encoding)(sys.stdout)
    if len(filenames) >= 1:
        fh_in = codecs.open(filenames[0], 'r', encoding)
    else:
        fh_in = codecs.getreader(encoding)(sys.stdin)
    return fh_in, fh_out


def process_sentences(func, filenames, encoding, options):
    """\
    Stream process given files or STDIN/STDOUT with the given function
    and encoding.
    """
    fh_in, fh_out = open_handles(filenames, encoding)
    
    #Red file sentences, there is one sentence per line
    sentences = fh_in.readlines()
    
    #Check of we need to restore the text structure
    if options.get('is_templ'):
        #Open the template file for reading
        with codecs.open(options['templ_file_name'], "r", encoding) as templ:
            #Get the text from the template file
            text = templ.read()
            #Declare the sentence index variable
            idx = 0
            #Process all the sentences but the last one
            for sentence in sentences:
                #Post-process the sentence
                sentence = func(sentence)
                #Remplace the placeholder with the sentence
                text = text.replace("{"+str(idx)+"}", sentence.strip(), 1)
                #Increment the index
                idx += 1

            #Put the text into the output file
            fh_out.write(text)
    else:
        #Process all the sentences but the last one
        for sentence in sentences[:-1]:
            print >> fh_out, func(sentence)

        #Process the last line but make sure we do not print it with the new line ending
        fh_out.write(func(sentences[-1]))

if __name__ == '__main__':
    # check on the number of arguments
    if len(sys.argv) == 0:
        display_usage()
        sys.exit(1)
    
    # parse options
    opts, filenames = getopt.getopt(sys.argv[1:], 'e:hcul:m:t:')
    options = {}
    help = False
    encoding = DEFAULT_ENCODING
    for opt, arg in opts:
        if opt == '-e':
            encoding = arg
        elif opt == '-l':
            options['language'] = arg.lower();
        elif opt == '-m':
            options['models_dir'] = arg;
        elif opt == '-c':
            options['is_capitalize'] = True
        elif opt == '-u':
            options['is_true_case'] = True
        elif opt == '-t':
            options['is_templ'] = True
            options['templ_file_name'] = arg
        elif opt == '-h':
            help = True
    
    # display help
    if help:
        display_usage()
        sys.exit(0)
        
    # the number of file names it too large
    if len(filenames) > 2:
        print "Improper list of arguments: ", sys.argv[1:]
        sys.exit(1)
        
    try:
        # process the input
        detok = PostProcessor(options)
        process_sentences(detok.post_process, filenames, encoding, options)
    except LookupError:
        print 'The NLTK post-processor does not support \'', options['language'].capitalize(), '\' language!'
        exit(1)
