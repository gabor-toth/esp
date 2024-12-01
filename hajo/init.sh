if [ $(basename $0) == "init.sh" ]; then
  echo "Source this file with '. ./$(basename $0)'"
  exit
fi

base_dir=$(readlink -f $(dirname ${BASH_SOURCE[0]}))

. $base_dir/../esp-idf/export.sh

echo "Set up git credentials"
export GIT_SSH_COMMAND="ssh -i $HOME/.ssh/id_gabtoth -o IdentitiesOnly=yes"
