/**
 * The BackPressure Routing Daemon (bprd).
 *
 * Copyright (c) 2012 Jeffrey Wildman <jeffrey.wildman@gmail.com>
 * Copyright (c) 2012 Bradford Boyle <bradford.d.boyle@gmail.com>
 *
 * bprd is released under the MIT License.  You should have received
 * a copy of the MIT License with this program.  If not, see
 * <http://opensource.org/licenses/MIT>.
 */

#ifndef __BACKLOGGER_H
#define __BACKLOGGER_H

extern void backlogger_thread_create();
extern void backlogger_packet_release(unsigned int count);

#endif /* __BACKLOGGER_H */
