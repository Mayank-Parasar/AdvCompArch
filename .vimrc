set nu
set guifont=Monospace\ 12
set hlsearch
"set cursorline
"colorscheme wombat256
highlight Normal guibg=black guifg=white
set background=dark
set tabstop=4       " The width of a TAB is set to 4.
                    " Still it is a \t. It is just that
                    " Vim will interpret it to be having
                    " a width of 4.

set shiftwidth=4    " Indents will have a width of 4
                    
set softtabstop=4   " Sets the number of columns for a TAB
                    
set expandtab       " Expand TABs to spaces
 
"folding settings
set foldmethod=indent   "fold based on indent
set foldnestmax=10      "deepest fold is 10 levels
set nofoldenable        "dont fold by default
set foldlevel=1         "this is just what i use

"Some common key bindings:
"`za` - toggles
"`zc` - closes
"`zo` - opens
"`zR` - open all
"`zM` - close all
